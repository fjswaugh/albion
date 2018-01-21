#pragma once

#include <vector>
#include <memory>
#include "function_input.h"

struct Statement;
struct Token;
struct Environment;
namespace Ast {
    struct Function;
}

struct Function {
    Function(const Ast::Function&, std::shared_ptr<Environment>);
    Function(Ast::Function&&, std::shared_ptr<Environment>);

    Function(const Function&) = delete;
    Function& operator=(const Function&) = delete;

    // Need to define these in .cpp file, otherwise there are difficulties in compiling due to
    // Statement being an incomplete type
    Function(Function&&);
    Function& operator=(Function&&);
    ~Function();

    const Ast::Function& expression() const;
    const std::shared_ptr<Environment>& closure() const;
    std::size_t id() const;
private:
    // This is only a pointer because of a circular dependency C++ issue meaning it can't be a
    // concrete type
    std::unique_ptr<Ast::Function> expression_;

    std::shared_ptr<Environment> closure_;
};

bool operator==(const Function&, const Function&);

#include <functional>
#include <string>

struct ObjectReference;

struct BuiltInFunction {
    std::string name;
    std::function<ObjectReference(const FunctionInput<ObjectReference>&, const Token&)> call;
};

