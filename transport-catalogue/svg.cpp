#include "svg.h"

#include <format>
#include <unordered_map>

namespace svg {

using namespace std::literals;

void Object::Render(const RenderContext& context) const {
    // Вывод отступа
    context.RenderIndent();

    // Вывод объекта
    RenderObject(context);

    // Вывод переноса строки
    context.out << std::endl;
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << std::format("<circle cx=\"{}\" cy=\"{}\" r=\"{}\" ", center_.x, center_.y, radius_);

    RenderAttrs(out);

    out << "/>";
}

// ---------- Polyline ------------------

Polyline& Polyline::AddPoint(Point point) {
    points_.push_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\""sv;

    for (size_t i = 0; i < points_.size(); ++i) {
        if (i != 0) {
            out << ' ';
        }
        out << points_[i];
    }

    out << "\" "sv; // Закрывающаяся скобка перечисления точек

    RenderAttrs(out);

    out << "/>"sv;

}

// ---------- Text ------------------

Text& Text::SetPosition(Point pos) {
    pos_ = pos;
    return *this;
}

Text& Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
}

Text& Text::SetFontSize(uint32_t size) {
    font_size_ = size;
    return *this;
}

Text& Text::SetFontFamily(std::string font_family) {
    font_family_ = std::move(font_family);
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight) {
    font_weight_ = std::move(font_weight);
    return *this;
}

Text& Text::SetData(std::string data) {
    static const std::unordered_map<char, std::string> kTegs = {
        {'\"', "&quot;"},   {'\'', "&apos;"},
        {'<', "&lt;"},      {'>', "&gt;"},
        {'&', "&amp;"}
    };

    std::string result;
    result.reserve(data.size() * 2);

    for (const auto c : data) {
        auto it = kTegs.find(c);
        
        if (it == kTegs.end()) {
            result += c;
        } else {
            result += it->second;
        }
    }

    std::swap(data_, result);
    return *this;
}

void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<text "sv;
    
    RenderAttrs(out);

    out << std::format(" x=\"{}\" y=\"{}\" ", pos_.x, pos_.y);
    out << std::format("dx=\"{}\" dy=\"{}\" ", offset_.x, offset_.y);
    out << std::format("font-size=\"{}\"", font_size_);

    if (font_family_.has_value()) {
        out << std::format(" font-family=\"{}\"", *font_family_);
    }

    if (font_weight_.has_value()) {
        out << std::format(" font-weight=\"{}\"", *font_weight_);
    }

    out << std::format(">{}</text>", data_);
}

// ---------- Document ------------------

void Document::AddPtr(std::unique_ptr<Object>&& obj) {
    objects_.emplace_back(std::move(obj));
}

void Document::Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
        << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n";
    
    RenderContext context(out, 1, 0);
    for (const auto& obj : objects_) {
        obj->Render(context);
    }

    out << "</svg>";
}

}  // namespace svg