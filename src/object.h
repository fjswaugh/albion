#pragma once

#include "general.h"
#include "function.h"

#include <variant>
#include <string>
#include <vector>
#include <string_view>
#include <iostream>
#include <memory>

struct Environment;
struct ObjectReference;

using Tuple = std::vector<ObjectReference>;
using Object =
    std::variant<std::nullptr_t, bool, double, std::string, Tuple, Function, BuiltInFunction>;

struct ObjectReference {
    ObjectReference(const ObjectReference&) = default;
    ObjectReference& operator=(const ObjectReference&) = default;
    ObjectReference(ObjectReference&&) = default;
    ObjectReference& operator=(ObjectReference&&) = default;

    template <
        typename T,
        typename = std::enable_if_t<std::is_same_v<std::decay_t<T>, ObjectReference> == false>>
    ObjectReference(T&& t) : data_{std::make_shared<Object>(std::forward<decltype(t)>(t))}
    {}

    template <typename T> bool holds() const noexcept
    {
        return std::holds_alternative<T>(*data_);
    }

    template <typename T> const T& get() const
    {
        if (!this->holds<T>()) throw Bad_access{};
        return std::get<T>(*data_);
    }

    template <typename F> decltype(auto) visit(F&& f) const
    {
        return std::visit(std::forward<F>(f), *data_);
    }

    struct Bad_access {};

    friend bool operator==(const ObjectReference&, const ObjectReference&);
    friend bool operator!=(const ObjectReference&, const ObjectReference&);
private:
    std::shared_ptr<Object> data_;
};

inline bool is_callable(const ObjectReference& o)
{
    return o.holds<Function>();
}

std::string to_string(const ObjectReference&);

inline void print(const ObjectReference& o)
{
    std::cout << to_string(o) << '\n';
}

inline bool is_truthy(const ObjectReference& o)
{
    if (o.holds<std::nullptr_t>()) return false;
    if (o.holds<bool>()) return o.get<bool>();
    return true;
}

