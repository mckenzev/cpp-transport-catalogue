#pragma once

#include <deque>
#include <vector>
#include <optional>
#include <unordered_map>
#include <set>
#include <string>
#include <string_view>

#include "geo.h"

class TransportCatalogue {

using string = std::string;
using string_view = std::string_view;

public:
	struct Stop;
	struct Bus {
		string name;				// Название автобуса
		std::vector<Stop*> stops; 	// Последовательный массив из указателей на остановки маршрута автобуса
	};

	struct Stop {
		// less предикат для сортировки указателей на автобусы по названию автобусов
		struct BusPredicate {
			constexpr bool operator()(const Bus* lhs, const Bus* rhs) const noexcept {
				return lhs->name < rhs->name;
			}
		};

		using BusesTable = std::set<Bus*, BusPredicate>;

		string name;				// Название остановки
		Coordinates coordinates; 	// Координаты остановки
		BusesTable buses; 			// Отсортированные по Bus::name указатели на автобусы
	};

	struct BusStat {
		size_t total_stops;			// Общее кол-во остановок
		size_t uniq_stops;			// Кол-во уникальных остановок
		double distance;			// Длина маршрута
	};

	/**
	 * Не смотря на наличие единственного атрибута структуры, для него выделена отдельная
	 * структура на случай, если статистику надо будет дополнить другими данными
	 */
	struct StopStat { 
		std::vector<string_view> buses_names;
	};

	void AddBus(string_view bus_name, std::vector<string_view> route);
	void AddStop(string_view stop_name, Coordinates coord);
	[[nodiscard]] std::optional<BusStat> BusInfo(string_view bus_id) const;
	[[nodiscard]] std::optional<StopStat> StopInfo(string_view stop_name) const;

private:

	std::deque<Stop> all_stops_;
	std::deque<Bus> all_buses_;

	// Ключ мапы ссылается на строку поля name структур `Stop` и `Bus`
	std::unordered_map<string_view, Stop*> stops_map_;
	std::unordered_map<string_view, Bus*> buses_map_;
	
	Stop* FindStop(string_view name);
	Bus* FindBus(string_view name);
};