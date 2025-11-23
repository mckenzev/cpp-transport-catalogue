#pragma once

#include <stack>

#include "json.h"

namespace json {

class Builder {
public:

/**
 * С флагами оптимизаций -O3 и -O2 и флагом -Werror вылетают ошибки -Wmaybe-uninitialized]
 * в этом месте "return BaseContext::Value(std::move(val));"
 * При этом с оптимизациями -O0 и -O1 все компилируется без проблем
 * Возможно это ложные срабатывания предупреждений из-за чрезмерных оптимизаций
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

    class DictItemContext;
    class DictContext;
    class ArrayContext;

    class BaseContext {
    public:
        BaseContext(Builder& builder) : builder_(builder) {}
        Node Build() {
            return builder_.Build();
        }

        DictItemContext Key(std::string key) {
            return builder_.Key(std::move(key));
        }

        BaseContext Value(Node::Value val) {
            return builder_.Value(std::move(val));
        }

        DictContext StartDict() {
            return builder_.StartDict();
        }

        ArrayContext StartArray() {
            return builder_.StartArray();
        }

        BaseContext EndDict() {
            return builder_.EndDict();
        }

        BaseContext EndArray() {
            return builder_.EndArray();
        }

    private:
        Builder& builder_;
    };

#pragma GCC diagnostic pop // #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"


    class DictItemContext final : public BaseContext {
    public:
        DictItemContext(BaseContext base) : BaseContext(base) {}
        Node Build() = delete;
        DictItemContext Key(std::string) = delete;
        BaseContext EndDict() = delete;
        BaseContext EndArray() = delete;
        DictContext Value(Node::Value val) {
            return BaseContext::Value(std::move(val));
        }
    };

    class DictContext final : public BaseContext {
    public:
        DictContext(BaseContext base) : BaseContext(base) {}
        Node Build() = delete;
        ArrayContext StartArray() = delete;
        BaseContext EndArray() = delete;
        BaseContext Value(Node::Value) = delete;
        DictContext StartDict() = delete;
    };

    class ArrayContext final : public BaseContext {
     public:
        ArrayContext(BaseContext base) : BaseContext(base) {}
        Node Build() = delete;
        DictItemContext Key(std::string) = delete;
        BaseContext EndDict() = delete;
        ArrayContext Value(Node::Value val) {
            return BaseContext::Value(std::move(val));
        }
    };


public:
    Builder();
    DictItemContext Key(std::string key);
    BaseContext Value(Node::Value val);
    DictContext StartDict();
    BaseContext EndDict();
    ArrayContext StartArray();
    BaseContext EndArray();
    Node Build();

private:
    Node root_node_;
    std::stack<Node*> node_stack_;

    BaseContext AddObject(Node::Value container, bool is_container);

    template <typename ContainerType>
    BaseContext CloseBracket();
};

} // namespace json