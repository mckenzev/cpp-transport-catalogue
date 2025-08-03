#pragma once

#include <iostream>
#include <cinttypes>
#include <map>
#include <string>
#include <vector>
#include <variant>

namespace json {

class Node;

using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

// Эта ошибка должна выбрасываться при ошибках парсинга JSON
class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class Node {
public:
    using NodeData = std::variant<int,
                                  double,
                                  std::string,
                                  bool,
                                  Array,
                                  Dict,
                                  std::nullptr_t>;

    Node() : Node(nullptr) {}
    Node(Array array);
    Node(Dict map);
    Node(std::string value);
    Node(int value);
    Node(double value);
    Node(bool value);
    Node(std::nullptr_t value);

    int AsInt() const;
    bool AsBool() const;
    double AsDouble() const;
    const std::string& AsString() const;
    const Array& AsArray() const;
    const Dict& AsMap() const;

    bool IsInt() const;
    bool IsBool() const;
    bool IsDouble() const; // Возвращает true, если в Node хранится int либо double.
    bool IsPureDouble() const; // Возвращает true, если в Node хранится double.
    bool IsString() const;
    bool IsNull() const;
    bool IsArray() const;
    bool IsMap() const;

    bool operator==(const Node& rhs) const;
    bool operator!=(const Node& rhs) const;

    void Print(std::ostream& out, uint8_t offset = 0) const;

private:
    NodeData data_;

    template <typename T>
    const T& AsRef() const;

    template <typename T>
    T AsVal() const;

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