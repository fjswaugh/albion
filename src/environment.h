#pragma once

#include "object.h"
#include "token.h"

#include <unordered_map>
#include <string>
#include <memory>

extern std::shared_ptr<Environment> global_environment;

struct Environment {
    Environment(std::shared_ptr<Environment> enclosing = nullptr)
        : enclosing_{std::move(enclosing)}
    {}

    void define(std::string name, ObjectReference value);

    void assign(const Token& token, ObjectReference value);
    void assign_at(const Token& token, ObjectReference value, int depth);

    const ObjectReference& get(const Token& token) const;
    const ObjectReference& get_at(const Token& token, int depth) const;
private:
    const Environment* ancestor(int distance) const;
    Environment* ancestor(int distance);

    std::shared_ptr<Environment> enclosing_;
    std::unordered_map<std::string, ObjectReference> values_;
};

