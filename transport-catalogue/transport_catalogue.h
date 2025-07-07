#pragma once

#include <deque>
#include <vector>
#include <optional>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <string_view>

#include "geo.h"

class TransportCatalogue {

using string = std::string;
using string_view = std::string_view;

public:
	struct Stop;
	struct Bus {
		string name;						// Название автобуса
		std::vector<const Stop*> stops; 	// Последовательный массив из указателей на остановки маршрута автобуса
	};

	struct Stop {
		string name;				// Название остановки
		Coordinates coordinates; 	// Координаты остановки
	};

	struct BusStat {
		size_t total_stops;			// Общее кол-во остановок
		size_t uniq_stops;			// Кол-во уникальных остановок
		double distance;			// Длина маршрута
	};

	using BusesTable = std::unordered_set<Bus*>;

	void AddBus(string_view bus_name, const std::vector<string_view>& route);
	void AddStop(string_view stop_name, Coordinates coord);
	const Bus* FindBus(string_view name) const;
	const Stop* FindStop(string_view name) const;
	[[nodiscard]] std::optional<BusStat> GetBusInfo(string_view bus_id) const;

	/**
	 * Возвращает `nullopt` если остановка не найдена или неотсортированный вектор(в т.ч. пустой) с остановками
	 */
	[[nodiscard]] std::optional<std::vector<string_view>> GetStopInfo(string_view stop_name) const;

private:
	std::deque<Stop> all_stops_;
	std::deque<Bus> all_buses_;

	// Ключ мапы ссылается на строку поля name структур `Stop` и `Bus`
	std::unordered_map<string_view, Stop*> stops_map_;
	std::unordered_map<string_view, Bus*> buses_map_;

	// Автобусы, проходящие через остановку
	// Ключ - название остановки, значение - множество автобусов, проходящих через остановку
	std::unordered_map<string_view, BusesTable> stop_to_buses_ ;
};