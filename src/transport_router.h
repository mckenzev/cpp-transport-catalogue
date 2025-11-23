#pragma once

#include <deque>
#include <optional>
#include <unordered_map>
#include <string>

#include "domain.h"
#include "transport_catalogue.h"
#include "router.h"


class TransportRouter {
public:

using Bus = domain::Bus;
using Stop = domain::Stop;
using RouteResponse = domain::dto::RouteResponse;
using RouteItem = domain::dto::RouteItem;
using Waiting = domain::dto::Waiting;
using Trip = domain::dto::Trip;

using Time = double;

// Структура для веса граней графа поможет хранить информацию о пройденных остановках и затраченного на это времени
struct GraphData {
    const Stop* start_stop; // Фактически от этих указателей нужна строка, но 16 байт на 2 указателя легче, чем 32 на 2 string_view
    const Bus* bus;
    Time spans_time;
    int wait_time;          // Из условия задачи "ожидание" - целое число, к тому же int легче double
    int span_count;


    constexpr bool operator<(const GraphData& rhs) const noexcept {
        return (spans_time + wait_time) < (rhs.spans_time + rhs.wait_time);
    }

    constexpr bool operator>(const GraphData& rhs) const noexcept {
        return (spans_time + wait_time) > (rhs.spans_time + rhs.wait_time);
    }

    GraphData operator+(const GraphData& rhs) const noexcept {
        return {
            .start_stop = nullptr,                      // В контексте суммирования объектов start_stop и bus
            .bus = nullptr,                             // не несут никакую смысловую нагрузку и их лучше занулить
            .spans_time = spans_time + rhs.spans_time,
            .wait_time = wait_time + rhs.wait_time,
            .span_count = span_count + rhs.span_count
        };
    }
};

using Graph = graph::DirectedWeightedGraph<GraphData>;

public:
    explicit TransportRouter(TransportCatalogue& db, domain::dto::RoutingSettings settings);
    std::optional<RouteResponse> GetRoute(std::string_view from, std::string_view to) const;

private:
    const TransportCatalogue& db_;
    domain::dto::RoutingSettings settings_;
    const std::deque<Stop>& all_stops_;
    std::unordered_map<const Stop*, graph::VertexId> vertices_id_;
    graph::DirectedWeightedGraph<GraphData> graph_;
    std::optional<graph::Router<GraphData>> router_;

    static constexpr double kMetersPerMinuteFactor = 1000.0 / 60.0;


    std::unordered_map<const Stop*, graph::VertexId> VerticesIdInitialization() const;
    void GraphInitialization();
    void AddEdgesInGraph(const std::vector<const Stop*>& stops_on_route, const Bus& bus);
    std::vector<Time> CreateTravelTimesVector(const std::vector<const Stop*>& stops_on_route) const;
    double CalculateTime(double distance) const noexcept;
    int GetDistance(const Stop* from, const Stop* to) const;
    RouteResponse BuildRouteResponse(const graph::Router<GraphData>::RouteInfo& route) const;
};