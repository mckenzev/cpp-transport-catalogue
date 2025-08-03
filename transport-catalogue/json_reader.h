#pragma once

#include <algorithm>
#include <iostream>
#include <vector>
#include <stdexcept>

#include "json.h"
#include "transport_catalogue.h"
#include "request_handler.h"

class JsonReader {
public:
    JsonReader(std::istream& input, std::ostream& output)
        : output_(output), doc_(json::Load(input)), handler_(db_) {}

    JsonReader(const JsonReader&) = delete;
    JsonReader& operator=(const JsonReader&) = delete;

    void ParseBaseRequests() {
        auto& all_requests = doc_.GetRoot().AsMap();
        auto& base_requests = all_requests.at("base_requests").AsArray();
        
        auto [stops_prop, buses_prop] = SplitRequests(base_requests);

        AddStops(stops_prop);
        SetRoadDistances(stops_prop);
        AddBuses(buses_prop);
    }

    void ParseStatRequests() {
        using namespace std::literals;
        auto& all_requests = doc_.GetRoot().AsMap();
        auto& stat_requests = all_requests.at("stat_requests").AsArray();
        json::Array responses;
        responses.reserve(stat_requests.size());

        for (auto& request : stat_requests) {
            auto request_prop = request.AsMap();
            int id = request_prop.at("id").AsInt();
            auto& type = request_prop.at("type").AsString();
            auto& name = request_prop.at("name").AsString();

            if (type == "Stop") {
                responses.push_back(GetStopStat(id, name));
            } else if (type == "Bus") {
                responses.push_back(GetBusStat(id, name));
            } else {
                throw std::runtime_error("Unable type \""s + type + "\" in \"stat_requests\" on json");
            }
        }
        json::Print(json::Document({responses}), output_);
    }

private:
    std::ostream& output_;
    json::Document doc_;
    TransportCatalogue db_;
    RequestHandler handler_;

    std::pair<std::vector<json::Dict>, std::vector<json::Dict>> SplitRequests(const json::Array& base_requests) const {
        using namespace std::literals;
        std::vector<json::Dict> stops_prop;
        std::vector<json::Dict> buses_prop;
        
        for (const auto& request : base_requests) {
            auto& request_prop = request.AsMap();
            auto& type = request_prop.at("type").AsString();
            if (type == "Bus") {
                buses_prop.emplace_back(request_prop);
            } else if (type == "Stop") {
                stops_prop.emplace_back(request_prop);
            } else {
                throw std::runtime_error("Unable type \""s + type + "\" in \"base_requests\" on json");
            }
        }

        return {std::move(stops_prop), std::move(buses_prop)};
    }

    void AddStops(const std::vector<json::Dict>& stops_prop) {
        for (const auto& stop : stops_prop) {
            std::string_view name = stop.at("name").AsString();
            double lat = stop.at("latitude").AsDouble();
            double lng = stop.at("longitude").AsDouble();
            handler_.AddStop(name, {lat, lng});
        }
    }

    void SetRoadDistances(const std::vector<json::Dict>& stops_prop) {
        for (const auto& stop : stops_prop) {
            std::string_view from = stop.at("name").AsString();
            auto& road_distances = stop.at("road_distances").AsMap();
            for (auto& [to_str, json_num] : road_distances) {
                auto distance = json_num.AsInt();
                std::string_view to = to_str;
                handler_.SetRoadDistance(from, to, distance);
            }
        }
    }

    std::vector<std::string_view> CreateRoute(const json::Array &stops, bool is_roundtrip) const {
        std::vector<std::string_view> result;
        size_t size = is_roundtrip ? stops.size() : stops.size() * 2 - 1;
        result.reserve(size);

        for (auto& stop : stops) {
            result.push_back(stop.AsString());
        }

        if (!is_roundtrip) {
            for (size_t i = result.size() - 1; i-- > 0;) {
                result.push_back(result[i]);
            }
        }

        return result;
    }

    void AddBuses(const std::vector<json::Dict>& buses_prop) {
        for (const auto& bus : buses_prop) {
            std::string_view name = bus.at("name").AsString();
            auto& stops = bus.at("stops").AsArray();
            
            // пустой список остановок из-за гарантий задания не попадется, но все же такая проверка есть
            if (stops.empty()) {
                handler_.AddBus(name, {});
                continue;
            }

            bool is_roundtrip = bus.at("is_roundtrip").AsBool();
            auto route = CreateRoute(stops, is_roundtrip);
            handler_.AddBus(name, route);
        }
    }

    json::Dict GetStopStat(int id, std::string_view name) const {
        json::Dict response;
        response["request_id"] = id;
        auto buses_table = handler_.GetBusesByStop(name);
        
        if (!buses_table.has_value()) {
            response["error_message"] = std::string("not found");
            return response;
        }

        std::vector<std::string> buses;
        buses.reserve(buses_table->size());
        for (auto bus : *buses_table) {
            buses.push_back(bus->name);
        }

        std::sort(buses.begin(), buses.end());

        response["buses"] = json::Array(std::make_move_iterator(buses.begin()), std::make_move_iterator(buses.end()));

        return response;
    }

    json::Dict GetBusStat(int id, std::string_view name) const {
        json::Dict response;
        response["request_id"] = id;
        auto bus_stat = handler_.GetBusStat(name);
        if (!bus_stat.has_value()) {
            response["error_message"] = std::string("not found");
            return response;
        }

        response["route_length"] = bus_stat->road_distance;
        response["stop_count"] = bus_stat->stop_count;
        response["unique_stop_count"] = bus_stat->uniq_stops;
        response["curvature"] = bus_stat->road_distance / bus_stat->geo_distance;

        return response;
    }
};
