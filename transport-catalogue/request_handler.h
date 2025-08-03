#pragma once

#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include "geo.h"
#include "transport_catalogue.h"
#include "domain.h"

/*
 * Здесь можно было бы разместить код обработчика запросов к базе, содержащего логику, которую не
 * хотелось бы помещать ни в transport_catalogue, ни в json reader.
 *
 * В качестве источника для идей предлагаем взглянуть на нашу версию обработчика запросов.
 * Вы можете реализовать обработку запросов способом, который удобнее вам.
 *
 * Если вы затрудняетесь выбрать, что можно было бы поместить в этот файл,
 * можете оставить его пустым.
 */

// Класс RequestHandler играет роль Фасада, упрощающего взаимодействие JSON reader-а
// с другими подсистемами приложения.
// См. паттерн проектирования Фасад: https://ru.wikipedia.org/wiki/Фасад_(шаблон_проектирования)

class RequestHandler {
public:
    using BusStat = domain::BusStat;
    using BusesTable = std::unordered_set<const domain::Bus*>;
    // MapRenderer понадобится в следующей части итогового проекта
    RequestHandler(TransportCatalogue& db/*, const renderer::MapRenderer& renderer*/)
        : db_(db) {}

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    // Возвращает информацию о маршруте (запрос Bus)
    std::optional<BusStat> GetBusStat(const std::string_view& bus_name) const {
        return db_.GetBusInfo(bus_name);
    }

    // Возвращает маршруты, проходящие через
    std::optional<BusesTable> GetBusesByStop(const std::string_view& stop_name) const {
        return db_.GetStopInfo(stop_name);
    }

    void AddBus(std::string_view name, const std::vector<std::string_view>& route) {
        db_.AddBus(name, route);
    }

    void AddStop(std::string_view name, geo::Coordinates coord) {
        db_.AddStop(name, coord);
    }

    void SetRoadDistance(std::string_view from, std::string_view to, int distance) {
        db_.SetRoadDistance(from, to, distance);
    }
    // Этот метод будет нужен в следующей части итогового проекта
    // svg::Document RenderMap() const;

private:
    // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
    TransportCatalogue& db_;
    // const renderer::MapRenderer& renderer_;
};
