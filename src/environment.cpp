#include "environment.h"
#include "function.h"
#include "error.h"
#include "object.h"
#include "token.h"
#include "function_input.h"

#include <string>
#include <cmath>

void Environment::define(std::string name, ObjectReference value)
{
    values_.insert_or_assign(std::move(name), std::move(value));
}

void Environment::assign(const Token& token, ObjectReference value) {
    const auto it = values_.find(token.lexeme);
    if (it == values_.end()) {
        if (enclosing_) {
            enclosing_->assign(token, std::move(value));
        } else {
            throw RuntimeError(token, "undefined variable '" + token.lexeme + "'");
        }
    } else {
        values_.insert_or_assign(token.lexeme, std::move(value));
    }
}

void Environment::assign_at(const Token& token, ObjectReference value, int depth)
{
    this->ancestor(depth)->assign(token, std::move(value));
}

const ObjectReference& Environment::get(const Token& token) const {
    const auto it = values_.find(token.lexeme);
    if (it == values_.end()) {
        if (enclosing_) return enclosing_->get(token);
        throw RuntimeError(token, "undefined variable '" + token.lexeme + "'");
    }
    return it->second;
}

const ObjectReference& Environment::get_at(const Token& token, int depth) const
{
    return this->ancestor(depth)->get(token);
}

const Environment* Environment::ancestor(int distance) const
{
    auto* environment = this;
    for (int i = 0; i < distance; ++i) {
        environment = environment->enclosing_.get();
    }
    return environment;
}

Environment* Environment::ancestor(int distance)
{
    auto* environment = this;
    for (int i = 0; i < distance; ++i) {
        environment = environment->enclosing_.get();
    }
    return environment;
}

