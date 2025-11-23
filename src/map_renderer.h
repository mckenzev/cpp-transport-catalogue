#pragma once

#include <optional>

#include "domain.h"
#include "geo.h"
#include "svg.h"

namespace renderer {
    using BusPtr = const domain::Bus*;
    using BusVec = std::vector<BusPtr>;
    using StopPtr = const domain::Stop*;
    using StopVec = std::vector<StopPtr>;
    using CoordVec = std::vector<geo::Coordinates>;


class MapRenderer {
private:
    class SphereProjector {
    public:
        template <typename PointInputIt>
        SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                        double max_width, double max_height, double padding);

        // Проецирует широту и долготу в координаты внутри SVG-изображения
        svg::Point operator()(geo::Coordinates coords) const;

    private:
        double padding_;
        double min_lon_ = 0;
        double max_lat_ = 0;
        double zoom_coeff_ = 0;
    };
    
    // Структура для перезаписи настроек с domain::dto:: на svg::,
    // чтобы в рантайме Point и Color постоянно не конвертировать в нужный формат,
    // что было бы, если бы в MapRenderer хранился domain::dto::RenderSettings, а не InternalRenderSettings
    struct InternalRenderSettings {
        double width;
        double height;
        double padding;
        double line_width;
        double stop_radius;
        double bus_label_font_size;
        double stop_label_font_size;
        double underlayer_width;
        svg::Point bus_label_offset;
        svg::Point stop_label_offset;
        svg::Color underlayer_color;
        std::vector<svg::Color>color_palette;
    };

public:
    MapRenderer() = default;

    void SetRenderSettings(domain::dto::RenderSettings&& settings);

    /**
     * Метод принимает отсортированные по name вектора с указателями на автобусы и остановки и возвращает изображение карты в виде строки в формате svg
     */
    std::string RenderMap(const BusVec& buses, const StopVec& stops) const;


private:
    std::optional<InternalRenderSettings> settings_;

    CoordVec GetStopsCoords(const StopVec& stops) const;
    SphereProjector CreateSphereProjector(const CoordVec& coords) const;
    void RenderPolylines(const BusVec& buses,const SphereProjector& proj, svg::Document& doc) const;
    void RenderBusNames(const BusVec& buses, const SphereProjector& proj, svg::Document& doc) const;
    void RenderStopsPoints(const StopVec& stops, const SphereProjector& proj, svg::Document& doc) const;
    void RenderStopsNames(const StopVec& stops, const SphereProjector& proj, svg::Document& doc) const;
};

} // namespace renderer