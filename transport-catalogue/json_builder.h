#pragma once

#include <stack>

#include "json.h"

namespace json {

class Builder {
public:
    class DictItemContext;
    class DictContext;
    class ArrayContext;

    class BaseContext {
    public:
        explicit BaseContext(Builder* ptr) : ptr_(ptr) {}
    protected:
        virtual ~BaseContext() = default;
        Builder* ptr_;
    };

    class DictItemContext final : public BaseContext {
    public:
        using BaseContext::BaseContext;

        DictContext Value(Node::Value val) {
            ptr_->Value(std::move(val));
            return DictContext(ptr_);
        }

        DictContext StartDict() {
            return ptr_->StartDict();
        }

        ArrayContext StartArray() {
            return ptr_->StartArray();
        }
    };

    class DictContext final : public BaseContext {
    public:
        using BaseContext::BaseContext;

        DictItemContext Key(std::string key) {
            return ptr_->Key(std::move(key));
        }

        Builder& EndDict() {
            return ptr_->EndDict();
        }
    };

    class ArrayContext final : public BaseContext {
     public:
        using BaseContext::BaseContext;

        ArrayContext& Value(Node::Value val) {
            ptr_->Value(std::move(val));
            return *this;
        }

        DictContext StartDict() {
            return ptr_->StartDict();
        }

        ArrayContext StartArray() {
            return ptr_->StartArray();
        }

        Builder& EndArray() {
            return ptr_->EndArray();
        }
    };


public:
    Builder();
    DictItemContext Key(std::string key);
    Builder& Value(Node::Value val);
    DictContext StartDict();
    Builder& EndDict();
    ArrayContext StartArray();
    Builder& EndArray();
    Node Build();

private:
    Node root_node_;
    std::stack<Node*> node_stack_;
};

} // namespace json