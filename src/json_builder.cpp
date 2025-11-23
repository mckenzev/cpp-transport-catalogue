#include "json_builder.h"

#include <type_traits>

using namespace std;
namespace json {

// При создании объекта Builder, указатель на корневой json объект попадает в стек, так как все методы работают с верхушкой стека
// Если стек опустел, значит объект построен, можно вызывать Build()
Builder::Builder()
    : root_node_(nullptr), node_stack_({&root_node_}) {}
    

Builder::DictItemContext Builder::Key(std::string key) {
    // Если стек пуст - объект построен, если на верхушке не словарь - работаем не в контексте словаря и Key() тут неуместен
    if (node_stack_.empty() || !node_stack_.top()->IsMap()) {
        throw std::logic_error("Key() call is only allowed inside dictionary context");
    }

    // Т.к. AsMap возвращает константную ссылку, есть необходимость убрать константность, т.к. его надо модифицировать
    Dict& dict = const_cast<Dict&>(node_stack_.top()->AsMap());

    // Новый ключ добавляется в словарь в паре с пустым объектом
    auto [it, inserted] = dict.insert({key, Node()});

    if (!inserted) {
        throw std::logic_error("Duplicate key");
    }

    // Пустой объект идет в стек, так как дальше придется модифицировать его (Value())
    node_stack_.push(&it->second);

    return BaseContext(*this);
}


Builder::BaseContext Builder::Value(Node::Value val) {
    return AddObject(move(val), /* is_container */ false);
}


/**
 * С оптимизацией -O3 и -O2 и флагом -Werror вылетают ошибки -Wmaybe-uninitialized]
 * в этом месте *top = move(object);
 * При этом с флагами оптимизаций -O0 и -O1 все компилируется без проблем
 * Возможно это ложные срабатывания предупреждений из-за чрезмерных оптимизаций
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

Builder::BaseContext Builder::AddObject(Node::Value object, bool is_container) {
    // Если стек пуст, то негде создавать новый объект
    if (node_stack_.empty()) {
        throw std::logic_error("Nested structure can only be started in array or dictionary context");
    }

    Node* top = node_stack_.top();
    if (top->IsArray()) {
        // Если на верхушке стека массив, снимаем константность и добавляем в него объект
        Array& json_array = const_cast<Array&>(top->AsArray());
        json_array.emplace_back(move(object));

        // Если добавлен контейнер, выводим его на верхушку стека для дальнейшей работы
        if (is_container) {
            node_stack_.push(&json_array.back());
        }
    } else {
        // Если на верхушке стека не массив, то просто перезаписывает значение верхушки
        *top = move(object);
        if (!is_container) {
            node_stack_.pop();
        }
    }
    
    return BaseContext(*this);
}

#pragma GCC diagnostic pop // #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"



template <typename ContainerType>
Builder::BaseContext Builder::CloseBracket() {
    // Если стек пуст, то ни для какого словаря/массива нет возможности закрыть скобку
    if (node_stack_.empty()) {
        throw std::logic_error("It is not possible to complete the creation of the dictionary because the dictionary has not been initialized");
    }

    Node::Value& top_val = node_stack_.top()->GetValue();

    if (!holds_alternative<ContainerType>(top_val)) {
        throw std::logic_error("An unpaired dictionary bracket has been detected");
    }

    // После закрытия скобки удаляем контейнер с верхушки стека
    node_stack_.pop();

    return *this;
}

Builder::DictContext Builder::StartDict() {
    return AddObject(Dict(), /* is_container */ true);
}

Builder::BaseContext Builder::EndDict() {
    return CloseBracket<Dict>();
}

Builder::ArrayContext Builder::StartArray() {
    return AddObject(Array(), /* is_container */ true);
}

Builder::BaseContext Builder::EndArray() {
    return CloseBracket<Array>();
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