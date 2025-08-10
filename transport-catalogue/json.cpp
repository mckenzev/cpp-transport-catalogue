#include "json.h"

#include <cctype>
#include <type_traits>

using namespace std;

namespace json {

namespace {

Node LoadNode(istream& input);

Node LoadArray(istream& input) {
    Array result;
    bool wait_comma = false;
    char c;
    
    while (input >> ws) {
        if (input.peek() == ']') {
            input.get(c);
            return Node(move(result));
        }

        if (wait_comma) {
            if (!input.get(c) || c != ',') {
                throw ParsingError("Between array items must be ','");
            }
        }
        
        result.emplace_back(LoadNode(input));
        wait_comma = true;
    }

    throw ParsingError("Unclosed array");
}

char ReadEscapeSequence(char c) {
    switch(c) {
        case 'n' : return '\n';
        case 'r' : return '\r';
        case '\"' : return '\"';
        case 't' : return '\t';
        case '\\' : return '\\';
        default: throw ParsingError("Invalid escape sequence");
    }

}

Node LoadString(istream& input) {
    string line;
    char c;
    while (input.get(c)) { // Выход из функции при закрытии скобки в цикле
        if (c == '\\') {
            if (!input.get(c)) throw ParsingError("Unfinished escape sequence");
            line += ReadEscapeSequence(c);
        } else if (c == '\"') {
            return Node(move(line));
        } else {
            line += c;
        }
    }
    // Если цикл завершился, а возврат строки не произошел - скобка не была закрыта, json некорректный
    throw ParsingError("Unclosed string");
}

Node LoadDict(istream& input) {
    Dict result;
    char c;
    bool wait_comma = false;
    
    // пока инпут не сломан, цикл будет продолжаться. Выход из цикла по break
    // На каждой итерации удаляются пробелы, т.к. после запятой они могут быть или на первой итерации перед }
    while (input >> ws) {
        if (input.peek() == '}') {
            input.get(c);
            return Node(move(result));
        }

        if (wait_comma) {
            if (!input.get(c) || c != ',') {
                throw ParsingError("Between dictionary items must be ','");
            }
        }

        string key = LoadNode(input).AsString(); // Ключ - строка. AsString кинет исключение если прочется не строка
        
        input >> ws;
        if (!input.get(c) || c != ':') {
            throw ParsingError("Invalid dictionary format");
        }

        // Если LoadNode попытается прочесть неверную ноду, она кинет исключение
        result.emplace(move(key), LoadNode(input));
        wait_comma = true;
    }

    throw ParsingError("Unclosed dictionary");
}

Node LoadNull(istream& input) {
    constexpr std::string_view kNullStr = "null";
    for (char c : kNullStr) {
        if (input.get() != c) {
            throw ParsingError("Expected 'null'");
        }
    }

    auto next = input.peek();
    static const string kValidTerminators = " ,}]:\n\t";
    if (next == EOF || kValidTerminators.find(static_cast<char>(next)) != kValidTerminators.npos) {
        return Node(nullptr);
    }
    throw ParsingError("Expected 'null'");
}

Node LoadBool(istream& input) {
    constexpr string_view kTrueStr = "true";
    constexpr string_view kFalseStr = "false";
    
    string_view compare_str;
    bool result;

    if (input.peek() == 't') {
        compare_str = kTrueStr;
        result = true;
    } else {
        compare_str = kFalseStr;
        result = false;
    }

    for (char c : compare_str) {
        if (input.get() != c) {
            throw ParsingError("text");
        }
    }

    auto next = input.peek();
    static const string kValidTerminators = " ,}]:\n\t";
    if (next == EOF || kValidTerminators.find(static_cast<char>(next)) != kValidTerminators.npos) {
        return Node(result);
    }
    throw ParsingError("text");
}

Node LoadNum(std::istream& input) {
    std::string parsed_num;

    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream");
        }
    };

    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected");
        }

        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }

    if (input.peek() == '0') {
        read_char();
    } else {
        read_digits();
    }

    bool is_int = true;
    // Парсим дробную часть числа
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    // Парсим экспоненциальную часть числа
    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            try {
                return std::stoi(parsed_num);
            } catch (...) {}
        }
        return std::stod(parsed_num);
    } catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

Node LoadNode(istream& input) {
    input >> ws;
    
    char c;
    if (!input.get(c)) {
        throw ParsingError("Invalid json format");
    }

    switch (c) {
        case '[' : return LoadArray(input);
        case '{' : return LoadDict(input);
        case 't' :
        case 'f' :
            input.putback(c);
            return LoadBool(input);
        case 'n' :
            input.putback(c);
            return LoadNull(input);
        case '"':
            return LoadString(input);
        default : 
            if (c == '-' || isdigit(c)) {
                input.putback(c);
                return LoadNum(input);
            }
            throw ParsingError("text");
    }
}

}  // namespace

// ------------- Node to ostream --------

struct Node::PrintNode {
    std::ostream& out;
    uint8_t offset;

    void PrintOffset(uint8_t value) const {
        out << string(value * 2, ' ');
    }

    template <typename T>
    void operator()(const T& val) const {
        Print(val);
    }

    void Print(int num) const {
        out << num;
    }

    void Print(double num) const {
        out << num;
    }

    void Print(bool val) const {
        out << (val ? "true" : "false");
    }

    void Print(nullptr_t) const {
        out << "null";
    }

    void Print(const Array& array) const {
        out << "[\n";
        // то, что находится в фигурных скобках, должно на 1 таб отступать от отступа блока массива
        PrintOffset(offset + 1);
        bool wait_comma = false;
        for (const auto& item : array) {
            if (wait_comma) {
                out << ",\n";
                // Так как произошел перенос строки отступ такой же, как при установке скобки
                PrintOffset(offset + 1);
            }
            // При принте следуюзих данных будет информация, что отступ отличается на 1 таб
            item.Print(out, offset + 1);
            wait_comma = true;
        }
        out << '\n';
        
        // отступ на уровне открывающейся скобки
        PrintOffset(offset);
        out << ']';
    }

    void Print(const Dict& dict) const {
        out << "{\n";
        PrintOffset(offset + 1);
        bool wait_comma = false;
        for (auto [key, value] : dict) {
            if (wait_comma) {
                out << ",\n";
                PrintOffset(offset + 1);
            }
            out << '"' << key << "\": ";
            value.Print(out, offset + 1);
            wait_comma = true;
        }
        out << '\n';
        PrintOffset(offset);
        out << '}';
    }

    string TranscriptEscapedSimbol(char c) const {
        switch (c) {
            case '\"': return "\\\"";
            case '\\': return "\\\\";
            case '\n': return "\\n";
            case '\r': return "\\r";
            case '\t': return "\\t";
            default: return string(1, c);
        }
    }

    void Print(const string& str) const {
        string escaped;
        escaped.reserve(str.size() * 1.1); // С эскейп последовательностями строка будет больше исходной
        for (const auto c : str) {
            escaped += TranscriptEscapedSimbol(c);
        }
        out << '\"' << escaped << '\"';
    }
};

// ------------- Node ---------------
/**
 * Array, Dict, string принимаются по значению, чтобы их можно было переместить в data_
 * Иначе при приеме по константной ссылке все равно приходилось бы производить копирование,
 * а для r-value пришлось бы сделать отдельную перегрузка
 */
Node::Node(Array array) : data_(move(array)) {}
Node::Node(Dict map) : data_(move(map)) {}
Node::Node(int value) : data_(value) {}
Node::Node(string value) : data_(move(value)) {}
Node::Node(double value) : data_(value) {}
Node::Node(bool value) : data_(value) {}
Node::Node(std::nullptr_t value) : data_(value) {}



template <typename T>
T Node::GetValueOrThrow(const string& type_name) const {
    try {
        return get<T>(data_);
    } catch (const bad_variant_access&) {
        throw logic_error("Exception when trying to convert Node to "s + type_name);
    }
}

template <typename T>
const T& Node::GetRefOrThrow(const string& type_name) const {
    try {
        return get<T>(data_);
    } catch (const bad_variant_access&) {
        throw logic_error("Exception when trying to convert Node to "s + type_name);
    }
}

int Node::AsInt() const {
    return GetValueOrThrow<int>("int");
}

bool Node::AsBool() const {
    return GetValueOrThrow<bool>("bool");
}

double Node::AsDouble() const {
    // Если значение хранится как double, оно будет возвращено как double
    if (IsPureDouble()) {
        return GetValueOrThrow<double>("double");
    }

    // Иначе получено как int и возвращено после преобразования в double
    return GetValueOrThrow<int>("double");
}

const string& Node::AsString() const {
    return GetRefOrThrow<string>("string");
}

const Array& Node::AsArray() const {
    return GetRefOrThrow<Array>("Array");
}

const Dict& Node::AsMap() const {
    return GetRefOrThrow<Dict>("Dict");
}

bool Node::IsInt() const {
    return holds_alternative<int>(data_);
}

bool Node::IsBool() const {
    return holds_alternative<bool>(data_);
}

// Возвращает true, если в Node хранится double.
bool Node::IsPureDouble() const {
    return holds_alternative<double>(data_);
}

// Возвращает true, если в Node хранится int либо double.
bool Node::IsDouble() const {
    return IsPureDouble() || IsInt();
} 

bool Node::IsString() const {
    return holds_alternative<string>(data_);
}

bool Node::IsNull() const {
    return holds_alternative<nullptr_t>(data_);
}

bool Node::IsArray() const {
    return holds_alternative<Array>(data_);
}

bool Node::IsMap() const {
    return holds_alternative<Dict>(data_);
}

constexpr bool Node::operator==(const Node& rhs) const noexcept {
    return data_ == rhs.data_;
}

constexpr bool Node::operator!=(const Node& rhs) const noexcept {
    return !(*this == rhs);
}

void Node::Print(std::ostream& out, uint8_t offset) const {
    visit(PrintNode{out, offset}, data_);
}

// ------------- Document ---------------

Document::Document(Node root)
    : root_(move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

bool Document::operator==(const Document& rhs) const {
    return GetRoot() == rhs.GetRoot();
}

bool Document::operator!=(const Document& rhs) const {
    return !(*this == rhs);
}

Document Load(istream& input) {
    return Document{LoadNode(input)};
}


void Print(const Document& doc, std::ostream& output) {
    doc.GetRoot().Print(output);
}

}  // namespace json