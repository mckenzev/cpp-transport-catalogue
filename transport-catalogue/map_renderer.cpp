#include "map_renderer.h"

#include <sstream>

using namespace domain;
using namespace domain::dto;
using namespace geo;
using namespace svg;
using namespace std;

namespace renderer {

namespace {

bool IsZero(double value) noexcept {
    constexpr double EPSILON = 1e-6;
    return abs(value) < EPSILON;
}

svg::Point ToSvgPoint(dto::Point point) noexcept {
    return {point.x, point.y};
}

svg::Color ToSvgColor(const dto::Color& color) {
    return visit([](const auto& arg) {
        using Type = decay_t<decltype(arg)>;
        if (is_same_v<Type, monostate>) {
            return svg::Color(monostate{});
        } else if constexpr (is_same_v<Type, string>) {
            return svg::Color(arg);
        } else if constexpr (is_same_v<Type, dto::Rgb>) {
            return svg::Color(svg::Rgb{.red = arg.red, .green = arg.green, .blue = arg.blue});
        } else if constexpr (is_same_v<Type, dto::Rgba>) {
            return svg::Color(svg::Rgba{.red = arg.red, .green = arg.green, .blue = arg.blue, .opacity = arg.opacity});
        }
    }, color);
}

} // namespace


template <typename PointInputIt>
MapRenderer::SphereProjector::SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                                              double max_width, double max_height, double padding)
                                            : padding_(padding) {
    // Если точки поверхности сферы не заданы, вычислять нечего
    if (points_begin == points_end) {
        return;
    }

    // Находим точки с минимальной и максимальной долготой
    const auto [left_it, right_it] = minmax_element(
        points_begin, points_end, [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
    
    min_lon_ = left_it->lng;
    const double max_lon = right_it->lng;

    // Находим точки с минимальной и максимальной широтой
    const auto [bottom_it, top_it] = minmax_element(
        points_begin, points_end, [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
    
    const double min_lat = bottom_it->lat;
    max_lat_ = top_it->lat;

    // Вычисляем коэффициент масштабирования вдоль координаты x
    optional<double> width_zoom;
    if (!IsZero(max_lon - min_lon_)) {
        width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
    }

    // Вычисляем коэффициент масштабирования вдоль координаты y
    optional<double> height_zoom;
    if (!IsZero(max_lat_ - min_lat)) {
        height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
    }

    if (width_zoom && height_zoom) {
        zoom_coeff_ = min(*width_zoom, *height_zoom);
    } else if (width_zoom) {
        zoom_coeff_ = *width_zoom;
    } else if (height_zoom) {
        zoom_coeff_ = *height_zoom;
    }
}

// Проецирует широту и долготу в координаты внутри SVG-изображения
svg::Point MapRenderer::SphereProjector::operator()(Coordinates coords) const {
    return {
        (coords.lng - min_lon_) * zoom_coeff_ + padding_,
        (max_lat_ - coords.lat) * zoom_coeff_ + padding_
    };
}

void MapRenderer::SetRenderSettings(RenderSettings&& settings) {
    vector<svg::Color> svg_color_palette;
    svg_color_palette.reserve(settings.color_palette.size());

    for (const auto& color : settings.color_palette) {
        svg_color_palette.push_back(ToSvgColor(color));
    }

    auto internal_settings = InternalRenderSettings{
        .width = settings.width,
        .height = settings.height,
        .padding = settings.padding,
        .line_width = settings.line_width,
        .stop_radius = settings.stop_radius,
        .bus_label_font_size = settings.bus_label_font_size,
        .stop_label_font_size = settings.stop_label_font_size,
        .underlayer_width = settings.underlayer_width,
        .bus_label_offset = ToSvgPoint(settings.bus_label_offset),
        .stop_label_offset = ToSvgPoint(settings.stop_label_offset),
        .underlayer_color = ToSvgColor(settings.underlayer_color),
        .color_palette = move(svg_color_palette)
    };

    settings_ = move(internal_settings);
}

/**
 * Метод принимает отсортированные по name вектора автобусов и остановок и возвращает изображение карты в виде строки в формате svg
 */
string MapRenderer::RenderMap(const BusVec& buses, const StopVec& stops) const {
    Document doc;
    auto stops_coords = GetStopsCoords(stops);
    auto proj = CreateSphereProjector(stops_coords);
    RenderPolylines(buses, proj, doc);
    RenderBusNames(buses, proj, doc);

    RenderStopsPoints(stops, proj, doc);
    RenderStopsNames(stops, proj, doc);

    ostringstream oss;
    doc.Render(oss);

    return oss.str();
}


CoordVec MapRenderer::GetStopsCoords(const StopVec& stops) const {
    CoordVec result;
    result.reserve(stops.size());

    for (const auto& stop : stops) {
        result.push_back(stop->coordinates);
    }

    return result;
}

MapRenderer::SphereProjector MapRenderer::CreateSphereProjector(const CoordVec& coords) const {
    if (!settings_.has_value()) {
        throw runtime_error("RenderSettings is not initialized");
    }

    return SphereProjector(coords.begin(), coords.end(), settings_->width, settings_->height, settings_->padding);
}

void MapRenderer::RenderPolylines(const BusVec& buses, const SphereProjector& proj, Document& doc) const {
    int color_idx = 0;
    for (const auto bus : buses) {
        Polyline route;
        route.SetFillColor("none"s)
             .SetStrokeColor(settings_->color_palette[color_idx % settings_->color_palette.size()])
             .SetStrokeWidth(settings_->line_width)
             .SetStrokeLineCap(StrokeLineCap::ROUND)
             .SetStrokeLineJoin(StrokeLineJoin::ROUND);

        const auto& stops = bus->stops;
        for (const auto stop : stops) {
            auto coord = proj(stop->coordinates);
            route.AddPoint(coord);
        }

        // Рисуем линию в обратном направлении для некольцевого маршрута
        if (!bus->is_roundtrip) {
            for (size_t i = stops.size() - 1; i-- > 0;) {
                auto coord = proj(stops[i]->coordinates);
                route.AddPoint(coord);
            }
        }
        
        doc.Add(move(route));
        ++color_idx;
    }
}


void MapRenderer::RenderBusNames(const BusVec& buses, const SphereProjector& proj, Document& doc) const {
    Text name;

    // Общие атрибуты для текста и подложки
    name.SetOffset(settings_->bus_label_offset)
        .SetFontSize(settings_->bus_label_font_size)
        .SetFontFamily("Verdana")
        .SetFontWeight("bold");
    
    Text substrate = name;

    // Уникальные атрибуты для подложки
    substrate.SetFillColor(settings_->underlayer_color)
             .SetStrokeColor(settings_->underlayer_color)
             .SetStrokeWidth(settings_->underlayer_width)
             .SetStrokeLineCap(StrokeLineCap::ROUND)
             .SetStrokeLineJoin(StrokeLineJoin::ROUND);

    const auto& color_palette = settings_->color_palette;
    int color_idx = 0;
    
    for (const auto bus : buses) {
        const auto& color = color_palette[color_idx % color_palette.size()];
        name.SetData(bus->name).SetFillColor(color);
        substrate.SetData(bus->name);
        
        const auto& stops = bus->stops;
        const auto coord = proj(stops.front()->coordinates);
        doc.Add(substrate.SetPosition(coord));
        doc.Add(name.SetPosition(coord));

        if (bus->is_roundtrip == false && stops.back() != stops.front()) {
            const auto coord = proj(stops.back()->coordinates);
            doc.Add(substrate.SetPosition(coord));
            doc.Add(name.SetPosition(coord));
        }

        ++color_idx;
    }
}


void MapRenderer::RenderStopsPoints(const StopVec& stops, const SphereProjector& proj, Document& doc) const {   
    Circle circle;
    circle.SetRadius(settings_->stop_radius).SetFillColor("white");

    for (const auto stop : stops) {
        const auto coord = proj(stop->coordinates);
        doc.Add(circle.SetCenter(coord));
    }
}

void MapRenderer::RenderStopsNames(const StopVec& stops, const SphereProjector& proj, Document& doc) const {
    Text text_name;
    text_name.SetOffset(settings_->stop_label_offset)
             .SetFontSize(settings_->stop_label_font_size)
             .SetFontFamily("Verdana")
             .SetFillColor("black");

    Text substrate = text_name;
    substrate.SetFillColor(settings_->underlayer_color)
             .SetStrokeColor(settings_->underlayer_color)
             .SetStrokeWidth(settings_->underlayer_width)
             .SetStrokeLineCap(StrokeLineCap::ROUND)
             .SetStrokeLineJoin(StrokeLineJoin::ROUND);

    for (const auto stop : stops) {
        const auto coord = proj(stop->coordinates);
        const auto& stop_name = stop->name;
        text_name.SetPosition(coord).SetData(stop_name);
        substrate.SetPosition(coord).SetData(stop_name);
        
        doc.Add(substrate);
        doc.Add(text_name);
    }
}

} // namespace renderer