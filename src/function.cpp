#include "function.h"
#include "token.h"
#include "ast.h"
#include "environment.h"

#include <memory>

Function::Function(const Ast::Function& f, std::shared_ptr<Environment> e)
    : expression_{std::make_unique<Ast::Function>(f)}, closure_{std::move(e)}
{}

Function::Function(Ast::Function&& f, std::shared_ptr<Environment> e)
    : expression_{std::make_unique<Ast::Function>(std::move(f))}, closure_{std::move(e)}
{}

const Ast::Function& Function::expression() const
{
    return *expression_;
}

const std::shared_ptr<Environment>& Function::closure() const
{
    return closure_;
}

std::size_t Function::id() const
{
    return reinterpret_cast<std::size_t>(this->expression().body.get());
}

Function::Function(Function&&) = default;
Function& Function::operator=(Function&&) = default;
Function::~Function() = default;

bool operator==(const Function& a, const Function& b)
{
    return a.id() == b.id();
}

