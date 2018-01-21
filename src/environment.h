#pragma once

#include "object.h"
#include "token.h"

#include <unordered_map>
#include <string>
#include <memory>

extern std::shared_ptr<Environment> global_environment;

struct Environment {
    Environment(std::shared_ptr<Environment> enclosing = global_environment)
        : enclosing_{std::move(enclosing)}
    {}

    void define(std::string name, ObjectReference value);
    void assign(const Token& token, ObjectReference value);
    const ObjectReference& get(const Token& token) const;
private:
    std::shared_ptr<Environment> enclosing_;
    std::unordered_map<std::string, ObjectReference> values_;
};

