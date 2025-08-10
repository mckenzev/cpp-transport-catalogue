#pragma once

#include <deque>
#include <vector>
#include <optional>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <string_view>

#include "domain.h"
#include "geo.h"

class TransportCatalogue {

using string = std::string;
using string_view = std::string_view;

public:
	using Bus = domain::Bus;
	using BusesTable = std::unordered_set<const Bus*>;
	using BusStat = domain::BusStat;
	using Stop = domain::Stop;

	void AddBus(string_view bus_name, const std::vector<string_view>& route, bool is_roundtrip);
	void AddStop(string_view stop_name, geo::Coordinates coord);

	/**
	 * Поиск автобуса. При отсутсвии возвращает `nullptr`
	 */
	const Bus* FindBus(string_view name) const;

	/**
	 * Поиск остановки. При отсутсвии возвращает `nullptr`
	 */
	const Stop* FindStop(string_view name) const;

	/**
	 * Возвращает nullopt если не удалось найти автобус
	 */
	[[nodiscard]] std::optional<BusStat> GetBusInfo(string_view bus_id) const;

	/**
	 * Возвращает `nullopt` если остановка не найдена или неотсортированный вектор(в т.ч. пустой) с остановками
	 */
	[[nodiscard]] std::optional<const BusesTable> GetStopStat(string_view stop_name) const;

	void SetRoadDistance(string_view from, string_view to, int distance);
	
	/**
	 * Поиск дорожного расстояния между остановками `from` и `to`
	 */
	std::optional<int> GetRoadDistance(string_view from, string_view to) const;

	/**
	 * Поиск Географического расстояния между остановками `from` и `to`
	 */
	std::optional<int> GetGeographicalDistance(string_view from, string_view to) const;

	const std::deque<Stop>& GetAllStops() const noexcept;

	const std::deque<Bus>& GetAllBuses() const noexcept;

private:
	std::deque<Stop> all_stops_;
	std::deque<Bus> all_buses_;

	// Ключ мапы ссылается на строку поля name структур `Stop` и `Bus`
	std::unordered_map<string_view, const Stop*> stops_map_;
	std::unordered_map<string_view, const Bus*> buses_map_;

	// Автобусы, проходящие через остановку
	// Ключ - название остановки, значение - множество автобусов, проходящих через остановку
	std::unordered_map<string_view, BusesTable> stop_to_buses_ ;

	// Алиас для хранения наименований остановок {.first = From, .second = To}
	using StopsPair = std::pair<string_view, string_view>;

	struct StopPairHasher {
		size_t operator()(const StopsPair& pair) const {
			return sv_hasher_(pair.first) + 17 * sv_hasher_(pair.second);
		}

	private:
		static constexpr std::hash<string_view> sv_hasher_{};
	};

	std::unordered_map<StopsPair, int, StopPairHasher> stops_distances_ ;
};