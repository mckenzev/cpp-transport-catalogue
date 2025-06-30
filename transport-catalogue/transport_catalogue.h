#pragma once

#include <deque>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <string>
#include <string_view>

#include "geo.h"

class TransportCatalogue {

using string = std::string;
using string_view = std::string_view;
using Route = std::vector<string_view>;
using BusesTable = std::set<string_view>;

public:
	void AddBus(string bus_name, Route route);
	void AddStop(string stop_name, Coordinates coord);
	string BusInfo(string_view bus_id) const;
	string StopInfo(string_view stop_name) const;

private:
	// Все строки в одном месте, на которые ссылаюстя string_veiew, содержимое которых может повторяться
	std::deque<string> all_buses_;
	std::deque<string> all_stops_;

	std::unordered_map<string_view, Coordinates> stops_coords_; // Координаты остановок
	std::unordered_map<string_view, Route> buses_; 				// Маршруты автобусов
	std::unordered_map<string_view, BusesTable> stops_info_;	// Проходящие через остановку автобусы

	/**
	 * @brief Получение маршрута из string_view, ссылающихся на строки из TransportCatalogue
	 * @param route вектор string_view, ссылающихся на строки из стека
	 */
	Route ProcessRouteStops(Route route);

	/**
	 * @brief Привязка маршрута в автобусу bus_name
	 */
	string_view RegisterBusName(string bus_name, Route route);

	/**
	 * Обновление информации остановок о проходящих через нее автобусах
	 */
	void UpdateStopInfo(string_view bus_name);
};