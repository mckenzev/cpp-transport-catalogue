#include "transport_catalogue.h"

#include <format>

using namespace std;

void TransportCatalogue::AddBus(string bus_name, Route route) {
    Route final_route = ProcessRouteStops(move(route));
    string_view bus = RegisterBusName(move(bus_name), move(final_route));
    UpdateStopInfo(bus);
}

void TransportCatalogue::AddStop(string stop_name, Coordinates coord) {
    auto it = stops_coords_.find(stop_name);
    if (it == stops_coords_.end()) {
        all_stops_.push_back(move(stop_name));
        // Если остановка впервые создается, то ее надо занести в stops_coords_ и stops_info_
        stops_coords_.emplace(all_stops_.back(), coord);
        stops_info_.emplace(all_stops_.back(), BusesTable());
    } else {
        // Иначе к существующей остановке привязать новые координаты
        it->second = coord;
    }
}

string TransportCatalogue::BusInfo(string_view bus_id) const {
    auto route = buses_.find(bus_id);
    // Если автобус с bus_id не найден, значит такого маршрута в каталоге нет
    if (route == buses_.end()) {
        return std::format("Bus {}: not found", bus_id);
    }

    // В связи с прошлым условием гарантируется, что route существует и в second пустая или непустая таблица имеется
    const auto& stops = route->second;

    size_t counter = 0;
    double distance = 0.;
    unordered_set<string_view> uniq_stops;

    // Если остановок в маршруте нет, вывод будет:
    // Bus {ID}: 0 stops on route, 0 unique stops, 0 route length
    // Иначе подсчет данных производится
    if (!stops.empty()) {
        Coordinates prev_stop = stops_coords_.at(stops.front());
        uniq_stops.insert(stops.front());
        ++counter;

        // обход массива с индекса 1, т.к. prev_stop не может быть инициализирован при индексе 0 
        // Если почему то у автобуса всего 1 остановка в маршруте, выводу будет 
        // Bus {ID}: 1 stops on route, 1 unique stops, 0 route length
        for (size_t i = 1; i < stops.size(); ++i) {
            Coordinates cur_stop = stops_coords_.at(stops[i]);
            distance += ComputeDistance(prev_stop, cur_stop);
            prev_stop = cur_stop;
            uniq_stops.insert(stops[i]);
            ++counter;
        }
    }

    return std::format("Bus {}: {} stops on route, {} unique stops, {:.6g} route length",
                        bus_id, counter, uniq_stops.size(), distance);
}

string TransportCatalogue::StopInfo(string_view stop_name) const {
    auto it = stops_info_.find(stop_name);
    if (it == stops_info_.end()) {
        return format("Stop {}: not found", stop_name);
    }

    if (it->second.empty()) {
        return format("Stop {}: no buses", stop_name);
    }
    
    const auto& buses = it->second;

    string result = format("Stop {}: buses", stop_name);

    // Подсчет итогового размера строки воизбежание реаллокаций с копированием
    size_t str_size = result.size();
    for (const auto bus_name : buses) {
        str_size += bus_name.size() + 1; // +1 для пробелов
    }

    result.reserve(str_size);
    
    for (const auto bus : buses) {
        result += ' ';
        result += bus;
    }

    return result;
}

// Вспомогательные методы из private

TransportCatalogue::Route TransportCatalogue::ProcessRouteStops(Route route) {
    Route final_route;
    final_route.reserve(route.size());
    // route - массив остановок (названия ссылаются на строки из стека), stop - название остановки в виде string_view(поэтому копия)
    for (auto stop : route) {
        auto it = stops_coords_.find(stop);
        // Если в unordered_map stops_coords_ найдено название stop, значит оно уже есть в deque all_stops_
        // Иначе оно добавляется в виде string в deque, а после, т.к. название в конце, из all_stops_ берется back() для получения string_view
        if (it == stops_coords_.end()) {
            all_stops_.push_back(string(stop));
            // Coordinates легкий обьект, поэтому проще и безопаснее таким образом инициализировать остановки без координат
            stops_coords_.emplace(all_stops_.back(), Coordinates(0., 0.));
            final_route.push_back(all_stops_.back());
        } else {
            final_route.push_back(it->first);
        }
    }

    return final_route;
}

string_view TransportCatalogue::RegisterBusName(string bus_name, Route route) {
    auto it = buses_.find(bus_name);
    // Если автобус не найден в buses_ значит его еще нет в deque и его надо туда внести, а после в buses_ отправить string_view на строку и маршрут
    if (it == buses_.end()) {
        all_buses_.push_back(move(bus_name));
        buses_.emplace(all_buses_.back(), move(route));
        return all_buses_.back();
    } 
    // Иначе поиск удачен и it указывает на нужную пару, где second - место храенния маршрута этого автобуса
    it->second = move(route);
    
    // Для дальнейшей регистрации автобуса на каждой остановке, метод возвращает string_view на название автобуса
    return it->first;
}

void TransportCatalogue::UpdateStopInfo(string_view bus_name) {
    const auto& stops = buses_[bus_name];
    for (const auto stop : stops) {
        stops_info_[stop].insert(bus_name);
    }
}