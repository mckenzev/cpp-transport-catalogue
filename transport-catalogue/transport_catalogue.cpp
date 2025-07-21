#include "transport_catalogue.h"

using namespace std;
using Bus = TransportCatalogue::Bus;
using Stop = TransportCatalogue::Stop;
using BusStat = TransportCatalogue::BusStat;
using BusesTable = TransportCatalogue::BusesTable;

void TransportCatalogue::AddBus(string_view bus_name, const vector<string_view>& route) {
    vector<const Stop*> final_route;
    final_route.reserve(route.size());

    for (auto name : route) {
        // К моменту создания автобусов остановки уже будут созданы, поэтому все FindStop вернут не nullptr
        // т.к. по ТЗ есть гарантия, что каждая остановка маршрута определена в некотором запросе Stop
        const Stop* stop = FindStop(name);
        final_route.push_back(stop);
    }
    
    Bus bus{string(bus_name), move(final_route)};
    all_buses_.push_back(move(bus));

    Bus* bus_ptr = &all_buses_.back();
    buses_map_.emplace(bus_ptr->name, bus_ptr);

    for (auto stop : bus_ptr->stops) {
        stop_to_buses_[stop->name].insert(bus_ptr);
    }
}

void TransportCatalogue::AddStop(string_view stop_name, Coordinates coord) {
    // Так как остановка может фигурировать как "соседняя" при создании другой ради указания географического маршрута,
    // эта "соседняя" остановка могла быть создана до ее официального создания через AddStop.
    // поэтому при официальном создании через AddStop проверяется, была ли создана остановка ранее или нет
    Stop stop{string(stop_name), coord};
    all_stops_.push_back(move(stop));
    Stop* stop_ptr = &all_stops_.back();
    stops_map_.emplace(stop_ptr->name, stop_ptr);
}

const Stop* TransportCatalogue::FindStop(string_view name) const {
    auto it = stops_map_.find(name);
    return it != stops_map_.end() ? it->second : nullptr;
}

const Bus* TransportCatalogue::FindBus(string_view name) const {
    auto it = buses_map_.find(name);
    return it != buses_map_.end() ? it->second : nullptr;
}

optional<BusStat> TransportCatalogue::GetBusInfo(string_view bus_id) const {
    const Bus* bus = FindBus(bus_id);
    
    if (bus == nullptr) {
        return nullopt;
    }

    const auto& stops = bus->stops;

    unordered_set<const Stop*> uniq_stops;
    // Размер контейнера точно будет не больше количества остановок, но преждевременная резервация убережет от реаллокаций
    uniq_stops.reserve(stops.size());
    double total_geo_distance = 0.;
    int total_road_distance = 0;
    
    if (!stops.empty()) {
        const Stop* prev = stops.front();
        uniq_stops.insert(stops.front());

        for (size_t i = 1; i < stops.size(); ++i) {
            uniq_stops.insert(stops[i]);
            const Stop* current = stops[i];
            // Для geo_distance проще напрямую вызывать ComputeDistance, чем GetGeographicalDistance,
            // т.к. в GetGeographicalDistance из sv опять придется получить указатель на Stop и лишь потом
            // получаются его координаты для рассчета растояния
            auto geo_distance = ComputeDistance(prev->coordinates, current->coordinates);
            total_geo_distance += geo_distance;

            // Если GetRoadDistance вернул не nullopt, то это значение суммируется с total_road_distance
            if (auto road_distance = GetRoadDistance(prev->name, current->name)) {
                total_road_distance += *road_distance;
            } else { // иначе дорожным расстоянием считается географическое
                total_road_distance += geo_distance;
            }
  
            prev = current;
        }
    }

    return BusStat{stops.size(), uniq_stops.size(), total_geo_distance, total_road_distance};
}

optional<const BusesTable> TransportCatalogue::GetStopInfo(string_view stop_name) const {
    if (FindStop(stop_name) == nullptr) {
        return nullopt;
    }

    auto it = stop_to_buses_.find(stop_name);
    return it == stop_to_buses_.end() ? BusesTable() : it->second;
}

void TransportCatalogue::SetRoadDistance(string_view from, string_view to, int distance) {
    // Необходимо создать пару string_view, которые ссылаются на оригинальные строки класса,
    // а не из стека, на которые ссылаются параметры функции
    const Stop* from_ptr = FindStop(from);
    const Stop* to_ptr = FindStop(to);

    if (from_ptr == nullptr || to_ptr == nullptr) {
        throw exception(); // Потом подобрать исключение получше
    }

    StopsPair stops_pair = {from_ptr->name, to_ptr->name};
    stops_distances_.emplace(stops_pair, distance);
}

std::optional<int> TransportCatalogue::GetGeographicalDistance(string_view from, string_view to) const {
    auto from_ptr = FindStop(from);
    auto to_ptr = FindStop(to);
    if (from_ptr == nullptr || to_ptr == nullptr) {
        return nullopt;
    }

    return ComputeDistance(from_ptr->coordinates, to_ptr->coordinates);
}

std::optional<int> TransportCatalogue::GetRoadDistance(string_view from, string_view to) const {
    // Если от A до B не удалось найти расстояние, ищем от B до A,
    // т.к в таком случае в обоих направлениях расстояния будут одинаковы

    // Поиск от A до B
    if (auto it = stops_distances_.find({from, to}); it != stops_distances_.end()) {
        return it->second;
    }

    // Поиск от B до A
    if (auto it = stops_distances_.find({to, from}); it != stops_distances_.end()) {
        return it->second;
    }

    // Если ни в одном направлении не удалось найти расстояне, значит оно не было указано
    return nullopt;
}