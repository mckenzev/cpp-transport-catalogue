#pragma once

#include <cinttypes>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>

namespace json {

class Node;

using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;


class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class Node final : private std::variant<std::nullptr_t, int, double, std::string, bool, Array, Dict> {
public:
    using Value = variant;
    using variant::variant;

    Value& GetValue();
    const Value& GetValue() const;

    int AsInt() const;
    bool AsBool() const;
    double AsDouble() const;
    const std::string& AsString() const;
    const Array& AsArray() const;
    const Dict& AsMap() const;

    bool IsInt() const;
    bool IsBool() const;
    bool IsDouble() const;      // Возвращает true, если в Node хранится int либо double.
    bool IsPureDouble() const;  // Возвращает true, если в Node хранится double.
    bool IsString() const;
    bool IsNull() const;
    bool IsArray() const;
    bool IsMap() const;

    constexpr bool operator==(const Node& rhs) const noexcept;
    constexpr bool operator!=(const Node& rhs) const noexcept;

    void Print(std::ostream& out, uint8_t offset = 0) const;

private:

    /**
     * Методы для As* методов, чтобы при ошибке при конвертации можно было увидеть для какого типа вылетело исключение
     */

    template <typename T>
    T GetValueOrThrow(const std::string& type_name) const;

    template <typename T>
    const T& GetRefOrThrow(const std::string& type_name) const;

    struct PrintNode;
};

class Document {
public:
    explicit Document(Node root);
    const Node& GetRoot() const;
    bool operator==(const Document& rhs) const;
    bool operator!=(const Document& rhs) const;

private:
    Node root_;
};

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

}  // namespace json