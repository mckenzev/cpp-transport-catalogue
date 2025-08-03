#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdint>
#include <format>
#include <stdexcept>
#include <deque>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>
#include <variant>
#include <string>
#include <unordered_map>

namespace svg {

struct Point {
    Point() = default;
    Point(double x, double y)
        : x(x)
        , y(y) {
    }
    double x = 0;
    double y = 0;
};

inline std::ostream& operator<<(std::ostream& out, const Point p) {
    out << p.x << ',' << p.y;
    return out;
}

/*
 * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
 * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
 */
struct RenderContext {
    RenderContext(std::ostream& out)
        : out(out) {
    }

    RenderContext(std::ostream& out, int indent_step, int indent = 0)
        : out(out)
        , indent_step(indent_step)
        , indent(indent) {
    }

    RenderContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }

    void RenderIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    std::ostream& out;
    int indent_step = 0;
    int indent = 0;
};

/*
 * Абстрактный базовый класс Object служит для унифицированного хранения
 * конкретных тегов SVG-документа
 * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
 */
class Object {
public:
    virtual ~Object() = default;
    void Render(const RenderContext& context) const;

private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};

struct Rgb {
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
};

inline std::ostream& operator<<(std::ostream& out, Rgb rgb) {
    out << std::format("rgb({},{},{})", rgb.red, rgb.green, rgb.blue);
    return out;
}

struct Rgba {
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    double opacity = 1.;
};

inline std::ostream& operator<<(std::ostream& out, Rgba rgba) {
    out << std::format("rgba({},{},{},{})", rgba.red, rgba.green, rgba.blue, rgba.opacity);
    return out;
}

using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;
inline const Color NoneColor = "none";

inline std::ostream& operator<<(std::ostream& out, Color color) {
    using namespace std;
    if (holds_alternative<monostate>(color)) {
        out << get<string>(NoneColor);
    } else if (holds_alternative<string>(color)){
        out << get<string>(color);
    } else if (holds_alternative<Rgb>(color)) {
        out << get<Rgb>(color);
    } else if (holds_alternative<Rgba>(color)) {
        out << get<Rgba>(color);
    } else {
        throw invalid_argument("Invalid color");
    }

    return out;
}

enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};

inline std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cap) {
    using namespace std::literals;
    static const std::unordered_map<StrokeLineCap, std::string_view> kLineCaps = {
        {StrokeLineCap::BUTT,	 "butt"sv},
        {StrokeLineCap::ROUND,	 "round"sv},
        {StrokeLineCap::SQUARE,  "square"sv}
    };

    auto it = kLineCaps.find(line_cap);
    
    if (it == kLineCaps.end()) {
        throw std::invalid_argument("Invalid parameter line_cap");
    }

    out << it->second;
    return out;
}

enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};

inline std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_join) {
    using namespace std::literals;
    static const std::unordered_map<StrokeLineJoin, std::string_view> kLineJoin = {
        {StrokeLineJoin::ARCS,	        "arcs"sv},
        {StrokeLineJoin::BEVEL,	        "bevel"sv},
        {StrokeLineJoin::MITER,         "miter"sv},
        {StrokeLineJoin::MITER_CLIP,    "miter-clip"sv},
        {StrokeLineJoin::ROUND,         "round"sv}
    };

    auto it = kLineJoin.find(line_join);
    
    if (it == kLineJoin.end()) {
        throw std::invalid_argument("Invalid parameter line_join");
    }
    
    out << it->second;
    
    return out;
}

template <typename Owner>
class PathProps {
public:
    Owner& SetFillColor(Color color) {
        fill_color_ = std::move(color);
        return AsOwner();
    }

    Owner& SetStrokeColor(Color color) {
        stroke_color_ = std::move(color);
        return AsOwner();
    }

    Owner& SetStrokeWidth(double width) {
        width_ = width;
        return AsOwner();
    }

    Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
        line_cap_ = line_cap;
        return AsOwner();
    }

    Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
        line_join_ = line_join;
        return AsOwner();
    }

protected:
    ~PathProps() = default;

    void RenderAttrs(std::ostream& out) const {
        using namespace std::literals;

        if (fill_color_) {
            out << " fill=\"" << *fill_color_ << "\"";
        }

        if (stroke_color_) {
            out << " stroke=\"" << *stroke_color_ << "\"";
        }

        if (width_) {
            out << " stroke-width=\"" << *width_ <<  "\"";
        }

        if (line_cap_) {
            out << " stroke-linecap=\"" << *line_cap_ <<  "\"";
        }

        if (line_join_) {
            out << " stroke-linejoin=\"" << *line_join_ <<  "\"";
        }
    }

private:
    std::optional<Color> fill_color_;
    std::optional<Color> stroke_color_;
    std::optional<double> width_;
    std::optional<StrokeLineCap> line_cap_;
    std::optional<StrokeLineJoin> line_join_;


    Owner& AsOwner() {
        return static_cast<Owner&>(*this);
    }
};

/*
 * Класс Circle моделирует элемент <circle> для отображения круга
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
 */
class Circle final : public Object, public PathProps<Circle> {
public:
    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);

private:
    void RenderObject(const RenderContext& context) const override;

    Point center_ = {0., 0.};
    double radius_ = 1.0;
};

/*
 * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
 */
class Polyline final : public Object, public PathProps<Polyline> {
public:
    Polyline& AddPoint(Point point);

private:
    void RenderObject(const RenderContext& context) const override;
    std::vector<Point> points_;
};

/*
 * Класс Text моделирует элемент <text> для отображения текста
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
 */
class Text final : public Object, public PathProps<Text> {
public:
    Text& SetPosition(Point pos);
    Text& SetOffset(Point offset);
    Text& SetFontSize(uint32_t size);
    Text& SetFontFamily(std::string font_family);
    Text& SetFontWeight(std::string font_weight);
    Text& SetData(std::string data);

private:
    void RenderObject(const RenderContext& context) const override;

    Point pos_ = {0., 0.};
    Point offset_ = {0., 0.};
    uint32_t font_size_ = 1u;
    std::optional<std::string> font_weight_;
    std::optional<std::string> font_family_;
    std::string data_;
};

class ObjectContainer {
public:
    template <typename T>
    void Add(T obj) {
        AddPtr(std::make_unique<T>(std::move(obj))); 
    }

    virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;

protected:
    ~ObjectContainer() = default;

    std::vector<std::unique_ptr<Object>> objects_;
};

class Document : public ObjectContainer {
public:
    /*
     Метод Add добавляет в svg-документ любой объект-наследник svg::Object.
     Пример использования:
     Document doc;
     doc.Add(Circle().SetCenter({20, 30}).SetRadius(15));
    */

    void AddPtr(std::unique_ptr<Object>&& obj) override;
    void Render(std::ostream& out) const;  
};

class Drawable {
public:
    virtual void Draw(ObjectContainer& container) const = 0;
    virtual ~Drawable() = default;
};

namespace shapes {

class Triangle : public Drawable {
public:
    Triangle(svg::Point p1, svg::Point p2, svg::Point p3)
        : p1_(p1)
        , p2_(p2)
        , p3_(p3) {
    }

    // Реализует метод Draw интерфейса svg::Drawable
    void Draw(ObjectContainer& container) const override {
        container.Add(Polyline().AddPoint(p1_).AddPoint(p2_).AddPoint(p3_).AddPoint(p1_));
    }

private:
    svg::Point p1_, p2_, p3_;
};

class Star : public Drawable {
public:
    Star(Point center, double outer_rad, double inner_rad, int num_rays)
        : center_(center), outer_rad_(outer_rad),
          inner_rad_(inner_rad), num_rays_(num_rays) {}

    void Draw(ObjectContainer& container) const override {
        Polyline star;
        for (int i = 0; i <= num_rays_; ++i) {
            double angle = 2 * M_PI * (i % num_rays_) / num_rays_;
            star.AddPoint({center_.x + outer_rad_ * sin(angle), center_.y - outer_rad_ * cos(angle)});
            
            if (i == num_rays_) {
                break;
            }

            angle += M_PI / num_rays_;
            star.AddPoint({center_.x + inner_rad_ * sin(angle), center_.y - inner_rad_ * cos(angle)});
        }

        star.SetFillColor("red").SetStrokeColor("black");
        container.Add(std::move(star));
    }

private:
    Point center_;
    double outer_rad_;
    double inner_rad_;
    int num_rays_;
};

class Snowman : public Drawable {
public:
    Snowman(Point center, double radius)
        : center_(center), radius_(radius) {}

    void Draw(ObjectContainer& container) const override {
        constexpr int circle_num = 3;
        std::deque<Circle> tmp_deq;
        Point center = center_;
        double radius = radius_;
        double offset = radius_ * 2;
        for (int i = 0; i < circle_num; ++i) {
            Circle circle = Circle().SetCenter(center)
                                    .SetRadius(radius)
                                    .SetFillColor("rgb(240,240,240)")
                                    .SetStrokeColor("black");
            tmp_deq.emplace_front(std::move(circle));
            center.y += offset;
            offset += radius_;
            radius += 0.5 * radius_;
        }
        
        for (auto&& a : tmp_deq) {
            container.Add(std::move(a));
        }
    }

private:
    Point center_;
    double radius_;
};

} // namespace shapes

}  // namespace svg