#include "transport_router.h"

using Bus = TransportRouter::Bus;
using Stop = TransportRouter::Stop;
using RouteResponse = TransportRouter::RouteResponse;
using RouteItem = TransportRouter::RouteItem;
using Waiting = TransportRouter::Waiting;
using Trip = TransportRouter::Trip;
using Time = TransportRouter::Time;
using GraphData = TransportRouter::GraphData;
using Graph = TransportRouter::Graph;

using namespace std;
using namespace graph;



TransportRouter::TransportRouter(TransportCatalogue& db, domain::dto::RoutingSettings settings)
    : db_(db),
      settings_(settings),
      all_stops_(db_.GetAllStops()),
      vertices_id_(VerticesIdInitialization()),
      graph_(GraphInitialization()),
      router_(graph_) {}

optional<RouteResponse> TransportRouter::GetRoute(string_view from, string_view to) const {
    VertexId from_id = vertices_id_.at(db_.FindStop(from));
    VertexId to_id = vertices_id_.at(db_.FindStop(to));

    auto route = router_.BuildRoute(from_id, to_id);

    if (!route.has_value()) {
        return std::nullopt;
    }

    return BuildRouteResponse(*route);
}

std::unordered_map<const Stop*, VertexId> TransportRouter::VerticesIdInitialization() const {
    std::unordered_map<const Stop*, VertexId> result;
    result.reserve(all_stops_.size());

    for (const auto& stop : all_stops_) {
        result[&stop] = result.size();
    }

    return result;
}

Graph TransportRouter::GraphInitialization() const {
    Graph graph(all_stops_.size());

    const auto& all_buses = db_.GetAllBuses();

    for (const auto& bus : all_buses) {
        const vector<const Stop*>& bus_route = bus.stops;
        
        AddEdgesInGraph(bus_route, graph, bus);

        if (!bus.is_roundtrip) {
            AddEdgesInGraph({bus_route.rbegin(), bus_route.rend()}, graph, bus);
        }
    }

    return graph;
}

void TransportRouter::AddEdgesInGraph(const vector<const Stop*>& stops_on_route, Graph& graph, const Bus& bus) const {
    // Вектор префиксных сумм времени, потраченного на путь из начала до конца маршрута
    vector<Time> travel_times = CreateTravelTimesVector(stops_on_route);

    // Добавлене всех отрезков пути в граф, где всего 1 ожидание и возможность проехать от 1-ой до всех остановок маршрута
    for (int i = 0; i < static_cast<int>(stops_on_route.size()); ++i) {
        const Stop* from_ptr = stops_on_route[i];
        VertexId from_id = vertices_id_.at(from_ptr);

        for (int j = i + 1; j < static_cast<int>(stops_on_route.size()); ++j) {
            const Stop* to_ptr = stops_on_route[j];
            VertexId to_id = vertices_id_.at(to_ptr);

            Time time = travel_times[j] - travel_times[i];
            int span_count = j - i;

            GraphData data {
                .spans_time = time,
                .wait_time = settings_.wait_time,
                .span_count = span_count,
                .start_stop = from_ptr,
                .bus = &bus
            };

            Edge<GraphData> edge{
                .from = from_id,
                .to = to_id,
                .weight = std::move(data)
            };
            
            graph.AddEdge(std::move(edge));
        }
    }
}


vector<Time> TransportRouter::CreateTravelTimesVector(const vector<const Stop*>& stops_on_route) const {
    // Префиксные суммы времени, необходимого для проезда по всему маршруту
    // Дальнейший доступ j - i даст время, необходимое потратить для проезда
    // из i в j
    // travel_time[j] - travel_time[i] = время на поездку из bus_route[i] в bus_route[j]
    vector<Time> travel_time(stops_on_route.size(), 0);

    for (size_t i = 1; i < stops_on_route.size(); ++i) {
        size_t from_idx = i - 1;
        size_t to_idx = i;

        const Stop* from_ptr = stops_on_route[from_idx];
        const Stop* to_ptr = stops_on_route[to_idx];

        double distance = GetDistance(from_ptr, to_ptr);
        Time time = CalculateTime(distance);
        travel_time[to_idx] = travel_time[from_idx] + time;
    }

    return travel_time;
}

double TransportRouter::CalculateTime(double distance) const noexcept {
    // velocity - дистанция в метрах, velocity преобразуется из км/ч в м/мин -> n мин
    return distance / (settings_.velocity * FACTOR_M_PER_MINUTE);
}


int TransportRouter::GetDistance(const Stop* from, const Stop* to) const {
    auto distance = db_.GetRoadDistance(from->name, to->name);
    
    // Если дорожную дистанцию не удалось найти, находим географическую
    if (!distance.has_value()) { 
        distance = db_.GetGeographicalDistance(from->name, to->name);
    }

    // т.к. работать приходится с существующими остановками, в distance какое то значение точно будет, можно на nullopt не проверять
    return *distance;
}

RouteResponse TransportRouter::BuildRouteResponse(const Router<GraphData>::RouteInfo& route) const {
    vector<RouteItem> items;
    const auto& edges = route.edges;
    
    for (auto edge_id : edges) {
        const auto& edge = graph_.GetEdge(edge_id);
        const GraphData& gd = edge.weight;

        Waiting waiting{
            .stop_name = gd.start_stop->name,
            .time = static_cast<int>(gd.wait_time)
        };

        items.emplace_back(std::move(waiting));

        Trip trip{
            .bus = gd.bus->name,
            .time = gd.spans_time,
            .span_count = gd.span_count
        };

        items.emplace_back(std::move(trip));
    }


    return RouteResponse{
        .items = std::move(items),
        .total_time = route.weight.spans_time +  route.weight.wait_time
    };
}
