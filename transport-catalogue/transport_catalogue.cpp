#include "transport_catalogue.h"

using namespace std;
using Bus = TransportCatalogue::Bus;
using Stop = TransportCatalogue::Stop;
using BusStat = TransportCatalogue::BusStat;

void TransportCatalogue::AddBus(string_view bus_name, const vector<string_view>& route) {
    vector<const Stop*> final_route;
    final_route.reserve(route.size());

    for (auto name : route) {
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

optional<vector<string_view>> TransportCatalogue::GetStopInfo(string_view stop_name) const {
    // Если остановка не зарегистрирована, значит ее нет - возвращается nullopt
    if (FindStop(stop_name) == nullptr) {
        return nullopt;
    }

    // Если остановка зарегистрирована, но не найдены автобусы, проходящие через нее - возвращается пустой массив
    auto it = stop_to_buses_.find(stop_name);
    if (it == stop_to_buses_.end()) {
        return vector<string_view>();
    }

    // Иначе из массива остановок формируется массив из наименований этих остановок
    const auto& buses = it->second;
    
    vector<string_view> names;
    names.reserve(buses.size());
    
    for (auto bus : buses) {
        names.push_back(bus->name);
    }

    return names;
}