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

const ObjectReference& Environment::get(const Token& token) const {
    const auto it = values_.find(token.lexeme);
    if (it == values_.end()) {
        if (enclosing_) return enclosing_->get(token);
        throw RuntimeError(token, "undefined variable '" + token.lexeme + "'");
    }
    return it->second;
}

#include <chrono>

std::shared_ptr<Environment> global_environment = [] {
    auto ge = std::make_shared<Environment>(nullptr);
    const auto begin = std::chrono::high_resolution_clock::now();
    auto clock = BuiltInFunction{
        "clock",
        [&begin](const FunctionInput<ObjectReference>&) -> ObjectReference {
            auto now = std::chrono::high_resolution_clock::now();
            return static_cast<double>(
                std::chrono::duration_cast<std::chrono::milliseconds>(now - begin).count());
        },
        0
    };
    auto read = BuiltInFunction{
        "read",
        [](const FunctionInput<ObjectReference>&) -> ObjectReference {
            std::string s;
            std::getline(std::cin, s);
            return s;
        },
        0
    };
    auto print = BuiltInFunction{
        "print",
        [](const FunctionInput<ObjectReference>& input) -> ObjectReference {
            if (input.size() == 0) std::cout << '\n';
            else std::cout << to_string(input[0]) << '\n';
            return nullptr;
        },
        1
    };

    ge->define("clock", std::move(clock));
    ge->define("read", std::move(read));
    ge->define("print", std::move(print));
    return ge;
}();

