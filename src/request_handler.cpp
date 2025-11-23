#include "request_handler.h"

#include <algorithm>

using namespace std;
using namespace geo;
using namespace domain;
using namespace domain::dto;

using BusesTable = TransportCatalogue::BusesTable;


optional<BusStat> RequestHandler::GetBusStat(const string& bus_name) const {
    return db_.GetBusInfo(bus_name);
}

optional<BusesTable> RequestHandler::GetStopStat(const string& stop_name) const {
    return db_.GetStopStat(stop_name);
}

void RequestHandler::AddBus(string_view name, const vector<string_view>& route, bool is_roundtrip) {
    db_.AddBus(name, route, is_roundtrip);
}

void RequestHandler::AddStop(string_view name, Coordinates coord) {
    db_.AddStop(name, coord);
}

void RequestHandler::SetRoadDistance(string_view from, string_view to, int distance) {
    db_.SetRoadDistance(from, to, distance);
}

void RequestHandler::SetRenderSettings(RenderSettings&& settings) {
    renderer_.SetRenderSettings(move(settings));
}

string RequestHandler::RenderMap() const {
    const auto& all_stops = db_.GetAllStops();
    vector<const Stop*> valid_stops;
    valid_stops.reserve(all_stops.size());
    for (auto& stop : all_stops) {
        // т.к. all_stops это корректные данные из db_, никогда не получится так,
        // что переданное в GetStopStat наименование не будет найдено и метод вернет nullopt
        if (!db_.GetStopStat(stop.name)->empty()) {
            valid_stops.emplace_back(&stop);
        }
    }

    auto& all_buses = db_.GetAllBuses();
    vector<const Bus*> valid_buses;
    valid_buses.reserve(all_buses.size());

    for (auto& bus : all_buses) {
        if (!bus.stops.empty()) {
           valid_buses.emplace_back(&bus); 
        }
    }

    auto comparator = [](const auto lhs, const auto rhs) -> bool {return lhs->name < rhs->name;};
    sort(valid_stops.begin(), valid_stops.end(), comparator);
    sort(valid_buses.begin(), valid_buses.end(), comparator);

    return renderer_.RenderMap(valid_buses, valid_stops);
}

void RequestHandler::RouterInitialization(RoutingSettings settings) {
    router_.emplace(db_, settings);
}

optional<RouteResponse> RequestHandler::BuildRoute(string_view from, string_view to) const {
    if (!router_.has_value()) {
        throw logic_error("Transport router is not initialized. Call RouterInitialization() first.");
    }

    return router_->GetRoute(from, to);
}