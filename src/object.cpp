#include "object.h"
#include "environment.h"
#include "interpreter.h"
#include "function.h"
#include "ast_printer.h"
#include "error.h"

bool operator==(const ObjectReference& a, const ObjectReference& b)
{
    return *a.data_ == *b.data_;
}

bool operator!=(const ObjectReference& a, const ObjectReference& b)
{
    return !(a == b);
}

std::string to_string(const ObjectReference& o)
{
    std::string s;
    const auto stringify = combine(
        [&s](std::nullptr_t) { s += "nil"; },
        [&s](bool x) { s += (x ? "true" : "false"); },
        [&s](double x) { s += std::to_string(x); },
        [&s](const std::string_view x) { s += x; },
        [&s](const Function& x) { s += "function " + std::to_string(x.id()); },
        [&s](const BuiltInFunction& x) { s += "built-in function " + x.name; },
        [&s](const Tuple& x) {
            s += "(";
            for (const auto& o : x) {
                s += to_string(o);
                s += ", ";
            }
            s.pop_back();
            s.pop_back();
            s += ")";
        },
        [&s](const Set& x) {
            s += "{";
            for (const auto& [name, o] : x) {
                s += name + " = " + to_string(o) + ", ";
            }
            s.pop_back();
            s.pop_back();
            s += "}";
        }
    );
    o.visit(stringify);
    return s;
}

