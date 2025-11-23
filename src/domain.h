#pragma once

#include <cstdint>
#include <variant>
#include <vector>
#include <string>

#include "geo.h"

namespace domain {

struct Stop;
struct Bus {
    std::string name;					// Название автобуса
    std::vector<const Stop*> stops; 	// Последовательный массив из указателей на остановки маршрута автобуса
    bool is_roundtrip;                  // Кольцевой маршрут?
};

struct Stop {
    std::string name;					// Название остановки
    geo::Coordinates coordinates; 		// Координаты остановки
};

struct BusStat {
    double geo_distance;				// Сумма геогрифических расстояний между остановками маршрута
    int stop_count;					    // Общее кол-во остановок
    int uniq_stops;					    // Кол-во уникальных остановок
    int road_distance;					// Сумма дорожных расстояний между остановками
};

namespace dto { // Объекты для передачи данных между несвязанными модулями

struct Point {
    double x;
    double y;
};

struct Rgb {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

struct Rgba {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    double opacity;
};

using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;

struct RenderSettings { // Настройки MapRenderer
    double width;
    double height;
    double padding;
    double line_width;
    double stop_radius;
    double bus_label_font_size;
    double stop_label_font_size;
    double underlayer_width;
    Point bus_label_offset;
    Point stop_label_offset;
    Color underlayer_color;
    std::vector<Color>color_palette;
};

struct RoutingSettings {
    double velocity;
    int wait_time;
};

// Структуры для хранения ответа из TransportRouter, который пройдя через RequestHandler должен использоваться в JsonReader
struct Waiting {
    std::string stop_name;
    int time;
};

struct Trip {
    std::string bus;
    double time;
    int span_count;
};

using RouteItem  = std::variant<Waiting, Trip>;

struct RouteResponse {
    std::vector<RouteItem> items;
    double total_time;
};

} // namespace dto

} // namespace domain

