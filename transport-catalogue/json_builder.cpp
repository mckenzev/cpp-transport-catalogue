#include "json_builder.h"


namespace json {

// При создании объекта Builder, указатель на корневой json объект попадает в стек, так как все методы работают с верхушкой стека
// Если стек опустел, значит объект построен, можно вызывать Build()
Builder::Builder()
    : root_node_(), node_stack_({&root_node_}) {}
    

Builder::DictItemContext Builder::Key(std::string key) {
    // Если стек пуст - объект построен, если на верхушке не словарь - работаем не в контексте словаря и Key() тут неуместен
    if (node_stack_.empty() || !node_stack_.top()->IsMap()) {
        throw std::logic_error("Key() call is only allowed inside dictionary context");
    }

    // Т.к. AsMap возвращает константную ссылку, есть необходимость убрать константность, т.к. его надо модифицировать
    Dict& dict = const_cast<Dict&>(node_stack_.top()->AsMap());

    if (dict.find(key) != dict.end()) {
        throw std::logic_error("Duplicate key");
    }

    // Новый ключ добавляется в словарь в паре с пустым объектом
    auto [it, flag] = dict.insert({key, Node()});

    // Пустой объект идет в стек, так как дальше придется модифицировать его (Value())
    node_stack_.push(&it->second);

    return DictItemContext(this);
}

Builder& Builder::Value(Node::Value val) {
    // Метод работает с верхним объектом стека, а именно присваивает ему значение параметра `val`
    // Если стек пуст, значит нечему передавать `val`
    if (node_stack_.empty()) {
        throw std::logic_error("Value() call is not allowed in current context");
    }

    // Если на верхушке стека массив, то просто пушим в него `val`
    if (node_stack_.top()->IsArray()) {
        auto& json_array = const_cast<Array&>(node_stack_.top()->AsArray());
        // Node наследник Node::Value, имеет такой же размер и выравнивание. Т.к. нет конвертации из
        // Node::Value в Node, приходится пользоваться реинтерпретацией
        json_array.push_back(reinterpret_cast<Node&>(val));
    } else { // В противном случае на верхушке указатель на пустой объект, которому надо присвоить значение `val`
        Node::Value& json_object = node_stack_.top()->GetValue();
        json_object = std::move(val);

        // Так как пустой объект больше не является таковым, из стека его можно убрать
        node_stack_.pop();
    }
    return *this;
}

Builder::DictContext Builder::StartDict() {
    // Если стек пуст, то негде создавать словарь
    if (node_stack_.empty()) {
        throw std::logic_error("Nested structure can only be started in array or dictionary context");
    }

    // Словарь может быть создан поверх пустого объекта(перезаписав его) или как элемент массива
    if (node_stack_.top()->IsArray()) {
        // Так как на вершине стека находится Node, которая на самом деле Array, получаем ссылку на массив,
        // убрав константность(у As* методов предусмотрен возврат только константных ссылок)
        Array& json_array = const_cast<Array&>(node_stack_.top()->AsArray());
        
        // * При записи json_array.push_back(Dict()) с оптимизацией -O3 не компилируется по причине неинициализации Dict(), хотя с -O0 все проходит,
        // поэтому сначала создаю дефолтный Dict в переменную, а после перезаписываю ее. Возможно оптимизатор сам оптимизирует это место раз так хочет что то оптимизировать
        Node dict;
        dict = Dict();
        json_array.push_back(std::move(dict));
        // Так как создался словарь, дальнейшая работа производится в контексте словаря
        node_stack_.push(&json_array.back());
    } else {
        Node::Value& json_object = node_stack_.top()->GetValue();
        json_object = Dict();
    }
    
    return DictContext(this);
}

Builder& Builder::EndDict() {
    // Если стек пуст, то ни для какого словаря нет возможности закрыть скобку
    if (node_stack_.empty()) {
        throw std::logic_error("It is not possible to complete the creation of the dictionary because the dictionary has not been initialized");
    }

    // Иначе если стек не пуст, на его вершине должен находиться указатель на словарь, для которого закрываем скобку
    if (!node_stack_.top()->IsMap()) {
        throw std::logic_error("An unpaired dictionary bracket has been detected");
    }

    // Так как словарь корректно создан, работу продолжаем с объектом, в контексте которого существовал словарь, а его указатель удаляем
    node_stack_.pop();

    return *this;
}

Builder::ArrayContext Builder::StartArray() {
    // Логика аналогичная StartDict()
    if (node_stack_.empty()) {
        throw std::logic_error("Nested structure can only be started in array or dictionary context");
    }

    if (node_stack_.top()->IsArray()) {
        auto& json_array = const_cast<Array&>(node_stack_.top()->AsArray());
        json_array.push_back(Array());
        node_stack_.push(&json_array.back());
    } else {
        Node::Value& json_object = node_stack_.top()->GetValue();
        json_object = Array();
    }
    
    return ArrayContext(this);
}

Builder& Builder::EndArray() {
    // И тут логика аналогична EndDict()
    if (node_stack_.empty()) {
        throw std::logic_error("It is not possible to complete the creation of the array because the array has not been initialized");
    }

    if (!node_stack_.top()->IsArray()) {
        throw std::logic_error("An unpaired array bracket has been detected");
    }

    node_stack_.pop();

    return *this;
}

Node Builder::Build() {
    // Если стек пуст, значит все объекты созданы корректно, иначе где то не закрыта скобка или корневой объект не был построен
    if (!node_stack_.empty()) {
        throw std::logic_error("It is impossible to build an incomplete object");
    }

    // т.к. root_node_ член класса, оптимизация NRVO не сработает, поэтому явно передаю через move готовый объект
    return std::move(root_node_);
}

} // namespace json