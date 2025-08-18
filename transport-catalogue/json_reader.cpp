#include "json_reader.h"

#include <algorithm>
#include <stdexcept>

using namespace std;
using namespace json;

JsonReader::JsonReader(istream& input, ostream& output)
    : output_(output), doc_(Load(input)) {
        auto render_settings = GetRenderSettings();
        handler_.SetRenderSettings(move(render_settings));
    }


void JsonReader::ParseBaseRequests() {
    Dict all_requests;
    all_requests = doc_.GetRoot().AsMap();
    auto& base_requests = all_requests.at("base_requests").AsArray();
    
    auto [stops_prop, buses_prop] = SplitRequests(base_requests);

    ParseStops(stops_prop);
    SetRoadDistances(stops_prop);
    ParseBuses(buses_prop);
}

void JsonReader::ParseStatRequests() {
    const auto& all_requests = doc_.GetRoot().AsMap();
    const auto& stat_requests = all_requests.at("stat_requests").AsArray();

    Builder builder;
    auto array = builder.StartArray();

    for (const auto& request : stat_requests) {
        const auto& request_prop = request.AsMap();
        int id = request_prop.at("id").AsInt();
        const auto& type = request_prop.at("type").AsString();
        
        if (type == "Stop") {
            const auto& name = request_prop.at("name").AsString();
            array.Value(MakeStopResponse(id, name).GetValue());
        } else if (type == "Bus") {
            const auto& name = request_prop.at("name").AsString();
            array.Value(MakeBusResponse(id, name).GetValue());
        } else if (type == "Map") {
            array.Value(RenderMap(id).GetValue());
        } else {
            throw std::runtime_error("Unable type \""s + type + "\" in \"stat_requests\" on json");
        }
    }

    auto json_object = array.EndArray().Build();
    json::Print(Document{std::move(json_object)}, output_);
}

pair<vector<Dict>, vector<Dict>> JsonReader::SplitRequests(const Array& base_requests) const {
    vector<Dict> stops_prop;
    vector<Dict> buses_prop;
    
    for (const auto& request : base_requests) {
        const auto& request_prop = request.AsMap();
        const auto& type = request_prop.at("type").AsString();
        if (type == "Bus") {
            buses_prop.emplace_back(request_prop);
        } else if (type == "Stop") {
            stops_prop.emplace_back(request_prop);
        } else {
            throw runtime_error("Unable type \""s + type + "\" in \"base_requests\" on json");
        }
    }

    return {move(stops_prop), move(buses_prop)};
}

void JsonReader::ParseStops(const vector<Dict>& stops_prop) {
    for (const auto& stop : stops_prop) {
        string_view name = stop.at("name").AsString();
        double lat = stop.at("latitude").AsDouble();
        double lng = stop.at("longitude").AsDouble();
        handler_.AddStop(name, {lat, lng});
    }
}

void JsonReader::SetRoadDistances(const vector<Dict>& stops_prop) {
    for (const auto& stop : stops_prop) {
        string_view from = stop.at("name").AsString();
        const auto& road_distances = stop.at("road_distances").AsMap();
        for (const auto& [to_str, json_object] : road_distances) {
            auto distance = json_object.AsInt();
            std::string_view to = to_str;
            handler_.SetRoadDistance(from, to, distance);
        }
    }
}

vector<string_view> JsonReader::CreateRoute(const Array &stops) const {
    // Результат функции в string_view, т.к. результат этой функции используется полностью
    // до выхода из области видимости вызывающей функции (AddBuses)
    // а создание sv из const string& проходит быстрее, чем создание string
    vector<string_view> result;
    size_t size = stops.size();
    result.reserve(size);

    for (const auto& stop : stops) {
        result.push_back(stop.AsString());
    }

    return result;
}

void JsonReader::ParseBuses(const vector<Dict>& buses_prop) {
    for (const auto& bus : buses_prop) {
        string_view name = bus.at("name").AsString();
        const auto& stops = bus.at("stops").AsArray();
        
        // пустой список остановок из-за гарантий задания не попадется, но все же такая проверка есть
        if (stops.empty()) {
            handler_.AddBus(name, {}, true);
            continue;
        }

        bool is_roundtrip = bus.at("is_roundtrip").AsBool();
        auto route = CreateRoute(stops);
        handler_.AddBus(name, route, is_roundtrip);
    }
}

Node JsonReader::MakeStopResponse(int id, const string& name) const {
    auto buses_table = handler_.GetStopStat(name);
    
    if (!buses_table.has_value()) {
        return Builder()
            .StartDict()
                .Key("request_id"s).Value(id)
                .Key("error_message"s).Value("not found"s)
            .EndDict()
        .Build();
    }

    vector<string> buses;
    buses.reserve(buses_table->size());

    for (const auto bus_ptr : *buses_table) {
        buses.push_back(bus_ptr->name);
    }

    sort(buses.begin(), buses.end());

    Builder builder;
    auto array = builder.StartArray();

    for (auto& bus : buses) {
        array.Value(std::move(bus));
    }
    
    Node::Value data = move(array.EndArray().Build().GetValue());

    return Builder()
        .StartDict()
            .Key("request_id"s).Value(id)
            .Key("buses"s).Value(move(data))
        .EndDict()
    .Build();
}

Node JsonReader::MakeBusResponse(int id, const string& name) const {
    auto stats = handler_.GetBusStat(name); 
    if (!stats.has_value()) {
        return Builder()
            .StartDict()
                .Key("request_id"s).Value(id)
                .Key("error_message"s).Value("not found"s)
            .EndDict()
        .Build();
    }

    return Builder()
        .StartDict()
            .Key("request_id"s).Value(id)
            .Key("route_length"s).Value(stats->road_distance)
            .Key("curvature"s).Value(stats->road_distance / stats->geo_distance)
            .Key("stop_count"s).Value(stats->stop_count)
            .Key("unique_stop_count"s).Value(stats->uniq_stops)
        .EndDict()
    .Build();
}

Node JsonReader::RenderMap(int id) const {
    string render_map = handler_.RenderMap();
    return Builder()
        .StartDict()
            .Key("request_id"s).Value(id)
            .Key("map"s).Value(move(render_map))
        .EndDict()
    .Build();
}

// Анонимное пространство имен для вспомогательных функций для парса
namespace {

domain::dto::Color ParseColor(const Node& json_object) {
    if (json_object.IsString()) {
        return json_object.AsString();
    }

    // Если под тегом цвета не строка, то обязательно должен быть json массив
    // Иначе AsArray кинет исключение
    auto& json_array = json_object.AsArray();

    if (json_array.empty()) {
        return {};
    }

    if (json_array.size() == 3) {
        return domain::dto::Rgb{
            .red = static_cast<uint8_t>(json_array[0].AsInt()),
            .green = static_cast<uint8_t>(json_array[1].AsInt()),
            .blue = static_cast<uint8_t>(json_array[2].AsInt())
        };
    }
    
    if (json_array.size() == 4) {
        return domain::dto::Rgba{
            .red = static_cast<uint8_t>(json_array[0].AsInt()),
            .green = static_cast<uint8_t>(json_array[1].AsInt()),
            .blue = static_cast<uint8_t>(json_array[2].AsInt()),
            .opacity = json_array[3].AsDouble()
        };
    }
    
    throw runtime_error("Invalid color parsing from json");
}

vector<domain::dto::Color> CreateColorPalette(const Array& json_array) {
    if (json_array.empty()) {
        throw invalid_argument("Color palette cannot be empty");
    }

    vector<domain::dto::Color> result;
    result.reserve(json_array.size());
    
    for (auto& json_object : json_array) {
        auto color = ParseColor(json_object);
        result.emplace_back(move(color));
    }

    return result;
}

domain::dto::Point ParsePoint(const Array& json_array) {
    if (json_array.size() == 2) {
        return domain::dto::Point{
            .x = json_array[0].AsDouble(),
            .y = json_array[1].AsDouble()
        };
    }

    throw runtime_error("Invalid point parsing from json");
}

} // namespace

domain::dto::RenderSettings JsonReader::GetRenderSettings() const {
    const auto& all_requests = doc_.GetRoot().AsMap();
    const auto& render_settings = all_requests.at("render_settings").AsMap();

    double width = render_settings.at("width").AsDouble();
    double height = render_settings.at("height").AsDouble();
    double padding = render_settings.at("padding").AsDouble();
    double line_width = render_settings.at("line_width").AsDouble();
    double stop_radius = render_settings.at("stop_radius").AsDouble();
    double bus_label_font_size = render_settings.at("bus_label_font_size").AsDouble();
    const Array& bus_label_offset = render_settings.at("bus_label_offset").AsArray();
    double stop_label_font_size = render_settings.at("stop_label_font_size").AsDouble();
    const Array& stop_label_offset = render_settings.at("stop_label_offset").AsArray();
    const Node& underlayer_color = render_settings.at("underlayer_color");
    double underlayer_width = render_settings.at("underlayer_width").AsDouble();
    const Array& color_palette = render_settings.at("color_palette").AsArray();

    return {
        .width = width,
        .height = height,
        .padding = padding,
        .line_width = line_width,
        .stop_radius = stop_radius,
        .bus_label_font_size = bus_label_font_size,
        .stop_label_font_size = stop_label_font_size,
        .underlayer_width = underlayer_width,
        .bus_label_offset = ParsePoint(bus_label_offset),
        .stop_label_offset = ParsePoint(stop_label_offset),
        .underlayer_color = ParseColor(underlayer_color),
        .color_palette = CreateColorPalette(color_palette)
    };
}