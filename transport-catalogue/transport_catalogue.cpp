#include "transport_catalogue.h"

#include <unordered_set>

using namespace std;
using Bus = TransportCatalogue::Bus;
using Stop = TransportCatalogue::Stop;
using BusStat = TransportCatalogue::BusStat;
using StopStat = TransportCatalogue::StopStat;


void TransportCatalogue::AddBus(string_view bus_name, vector<string_view> route) {
    vector<Stop*> final_route;
    final_route.reserve(route.size());

    for (auto name : route) {
        Stop* stop = FindStop(name);
        final_route.push_back(stop);
    }
    
    Bus* bus = FindBus(bus_name);
    bus->stops = move(final_route);

    for (auto stop : bus->stops) {
        stop->buses.insert(bus);
    }
}

void TransportCatalogue::AddStop(string_view stop_name, Coordinates coord) {
    auto stop = FindStop(stop_name);
    stop->coordinates = coord;
}

Stop* TransportCatalogue::FindStop(string_view name) {
    auto it = stops_map_.find(name);
    if (it != stops_map_.end()) {
        return it->second; 
    }

    all_stops_.push_back(Stop{string(name), {}, {}});
    Stop* new_stop = &all_stops_.back();
    stops_map_.emplace(new_stop->name, new_stop);

    return new_stop;
}

Bus* TransportCatalogue::FindBus(string_view name) {
    auto it = buses_map_.find(name);
    if (it != buses_map_.end()) {
        return it->second;
    }
    
    all_buses_.push_back(Bus{string(name), {}});
    Bus* new_bus = &all_buses_.back();
    buses_map_.emplace(new_bus->name, new_bus);

    return new_bus;
    
}

optional<BusStat> TransportCatalogue::BusInfo(string_view bus_id) const {
    auto bus = buses_map_.find(bus_id);
    
    if (bus == buses_map_.end()) {
        return nullopt;
    }

    const auto& stops = bus->second->stops;

    unordered_set<Stop*> uniq_stops;
    // Размер контейнера точно будет не больше количества остановок, но преждевременная резервация убережет от реаллокаций
    uniq_stops.reserve(stops.size());
    double distance = 0.;
    
    if (!stops.empty()) {
        Coordinates prev = stops.front()->coordinates;
        uniq_stops.insert(stops.front());

        for (size_t i = 1; i < stops.size(); ++i) {
            uniq_stops.insert(stops[i]);
            Coordinates current = stops[i]->coordinates;
            distance += ComputeDistance(prev, current);
            prev = current;
        }
    }

    return BusStat{stops.size(), uniq_stops.size(), distance};
}

optional<StopStat> TransportCatalogue::StopInfo(string_view stop_name) const {
    auto stop = stops_map_.find(stop_name);
    if (stop == stops_map_.end()) {
        return nullopt;
    }

    const auto& buses = stop->second->buses;
    
    vector<string_view> names;
    names.reserve(buses.size());
    
    for (auto bus : buses) {
        names.push_back(bus->name);
    }

    return StopStat{move(names)};
}