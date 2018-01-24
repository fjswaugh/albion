#include "resolver.h"
#include <stack>
#include <vector>
#include <cassert>
#include <unordered_set>

void Scope::define(const Ast::VariableTuple& vt)
{
    for_each_variable(vt, [this](const Ast::Variable& v) { data_.insert(v.name.lexeme); });
}

bool Scope::has_defined(const Ast::Variable& v) const
{
    return data_.find(v.name.lexeme) != data_.end();
}

std::optional<int> ScopeStack::resolve(const Ast::Variable& v) const
{
    for (std::size_t i = 0; i < this->size(); ++i) {
        if (this->at(i).has_defined(v)) {
            return i;
        }
    }
    return {};
}

struct Resolver : Ast::Expression::Visitor<void>, Ast::Statement::Visitor<void> {
    Resolver(ScopeStack& ss, Locations& l)
        : scopes_{ss}, locations_{l}
    {}

    Resolver(const Resolver&) = delete;
    Resolver(Resolver&&) = delete;

    void resolve(const Ast::Ast& ast);

    void operator()(const Ast::Assign&) override;
    void operator()(const Ast::Binary&) override;
    void operator()(const Ast::Call&) override;
    void operator()(const Ast::Function&) override;
    void operator()(const Ast::Grouping&) override;
    void operator()(const Ast::Literal&) override;
    void operator()(const Ast::Logical&) override;
    void operator()(const Ast::Tuple&) override;
    void operator()(const Ast::Unary&) override;
    void operator()(const Ast::Variable&) override;
    void operator()(const Ast::VariableTuple&) override;

    void operator()(const Ast::Block&) override;
    void operator()(const Ast::ExpressionStatement&) override;
    void operator()(const Ast::If&) override;
    void operator()(const Ast::Return&) override;
    void operator()(const Ast::While&) override;
    void operator()(const Ast::Declaration&) override;

private:
    ScopeStack& scopes_;
    Locations& locations_;
};

void Resolver::operator()(const Ast::Assign& a)
{
    a.expression->accept(*this);
    for_each_variable(*a.variable, [this](const Ast::Variable& v) {
        if (const auto location = scopes_.resolve(v)) {
            locations_[&v] = *location;
        }
    });
}

void Resolver::operator()(const Ast::Binary& b)
{
    b.left->accept(*this);
    b.right->accept(*this);
}

void Resolver::operator()(const Ast::Call& c)
{
    c.callee->accept(*this);

    for (std::size_t i = 0; i < c.input.size(); ++i) {
        c.input[i]->accept(*this);
    }
}

void Resolver::operator()(const Ast::Function& f)
{
    scopes_.push();
    for (std::size_t i = 0; i < f.input.size(); ++i) {
        scopes_.top().define(*f.input[i]);
    }
    f.body->accept(*this);
    scopes_.pop();
}

void Resolver::operator()(const Ast::Grouping& g)
{
    g.expression->accept(*this);
}

void Resolver::operator()(const Ast::Literal&)
{
    // A literal expression doesn't mention any variables, nor does it contain
    // any subexpressions, so there is nothing to resolve
}

void Resolver::operator()(const Ast::Logical& l)
{
    l.left->accept(*this);
    l.right->accept(*this);
}

void Resolver::operator()(const Ast::Tuple& t)
{
    for (const auto& e : t.elements) {
        e->accept(*this);
    }
}

void Resolver::operator()(const Ast::Unary& u)
{
    u.right->accept(*this);
}

void Resolver::operator()(const Ast::Variable& v)
{
    if (const auto location = scopes_.resolve(v)) {
        locations_[&v] = *location;
    }
}

void Resolver::operator()(const Ast::VariableTuple&)
{
    assert(false && "Shouldn't need to evaluate an Ast::VariableTuple, they're just part of "
                    "assignments and variable declarations");
}

void Resolver::operator()(const Ast::Block& b)
{
    scopes_.push();
    this->resolve(b.statements);
    scopes_.pop();
}

void Resolver::operator()(const Ast::ExpressionStatement& es)
{
    if (es.expression) {
        (*es.expression)->accept(*this);
    }
}

void Resolver::operator()(const Ast::If& if_statement)
{
    if_statement.condition->accept(*this);
    if_statement.then_branch->accept(*this);
    if (if_statement.else_branch) {
        (*if_statement.else_branch)->accept(*this);
    }
}

void Resolver::operator()(const Ast::Return& r)
{
    if (r.expression) {
        (*r.expression)->accept(*this);
    }
}

void Resolver::operator()(const Ast::While& w)
{
    w.condition->accept(*this);
    w.body->accept(*this);
}

void Resolver::operator()(const Ast::Declaration& d)
{
    if (d.initializer) {
        (*d.initializer)->accept(*this);
    }

    scopes_.top().define(*d.variable);
}

void Resolver::resolve(const Ast::Ast& ast)
{
    for (const auto& statement : ast) {
        statement->accept(*this);
    }
}

Locations resolve(const Ast::Ast& ast, ScopeStack& scopes)
{
    Locations locations;
    Resolver r{scopes, locations};
    r.resolve(ast);
    return locations;
}

