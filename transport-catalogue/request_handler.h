#pragma once

#include <optional>
#include <vector>
#include <string>

#include "domain.h"
#include "geo.h"
#include "map_renderer.h"
#include "transport_catalogue.h"
#include "transport_router.h"


class RequestHandler {
public:
    using BusStat = domain::BusStat;
    using BusesTable = TransportCatalogue::BusesTable;
    
    RequestHandler() = default;
    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    std::optional<BusStat> GetBusStat(const std::string& bus_name) const;
    std::optional<BusesTable> GetStopStat(const std::string& stop_name) const;
    void AddBus(std::string_view name, const std::vector<std::string_view>& route, bool is_roundtrip);
    void AddStop(std::string_view name, geo::Coordinates coord);
    void SetRoadDistance(std::string_view from, std::string_view to, int distance);

    // Запросы на рендер карты
    void SetRenderSettings(domain::dto::RenderSettings&& settings);
    std::string RenderMap() const;

    // Запросы на поиск маршрута
    void RouterInitialization(domain::dto::RoutingSettings settings);
    std::optional<domain::dto::RouteResponse> BuildRoute(std::string_view from, std::string_view to) const;

private:
    /**
     * RequestHandler владеет транспортным каталогом и рендером, чтобы JsonReder ничего не знал об этих модулях и занимался только чтением json документа и отправкой запросы в хэндлер
     */

    TransportCatalogue db_;
    renderer::MapRenderer renderer_;
    std::optional<TransportRouter> router_; // Инициализируется при первом запросе на поиск маршрута
};
