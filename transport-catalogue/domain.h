#pragma once

#include <cstdint>
#include <variant>
#include <vector>
#include <string>

#include "geo.h"
/*
 * В этом файле вы можете разместить классы/структуры, которые являются частью предметной области (domain)
 * вашего приложения и не зависят от транспортного справочника. Например Автобусные маршруты и Остановки. 
 *
 * Их можно было бы разместить и в transport_catalogue.h, однако вынесение их в отдельный
 * заголовочный файл может оказаться полезным, когда дело дойдёт до визуализации карты маршрутов:
 * визуализатор карты (map_renderer) можно будет сделать независящим от транспортного справочника.
 *
 * Если структура вашего приложения не позволяет так сделать, просто оставьте этот файл пустым.
 *
 */

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

} // namespace dto

} // namespace domain

