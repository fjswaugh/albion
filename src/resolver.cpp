#include "resolver.h"
#include <stack>
#include <vector>
#include <cassert>
#include <unordered_set>

struct Scope {
    Scope() = default;

    void define(const Ast::VariableTuple&);
    bool has_defined(const Ast::Variable&) const;
private:
    std::unordered_set<std::string> data_;
};

void Scope::define(const Ast::VariableTuple& vt)
{
    for_each_variable(vt, [this](const Ast::Variable& v) { data_.insert(v.name.lexeme); });
}

bool Scope::has_defined(const Ast::Variable& v) const
{
    return data_.find(v.name.lexeme) != data_.end();
}

struct ScopeStack {
    ScopeStack() = default;

    auto size() const { return data_.size(); }
    bool empty() const { return data_.empty(); }

    void push() { data_.emplace_back(); }
    void pop() { data_.pop_back(); }

    Scope& top() { assert((!data_.empty())); return data_.back(); }
    const Scope& top() const { return data_.back(); }

    int resolve(const Ast::Variable&) const;
private:
    Scope& at(std::size_t i) { return data_[data_.size() - i - 1]; }
    const Scope& at(std::size_t i) const { return data_[data_.size() - i - 1]; }

    std::vector<Scope> data_;
};

int ScopeStack::resolve(const Ast::Variable& v) const
{
    for (std::size_t i = 0; i < this->size(); ++i) {
        if (this->at(i).has_defined(v)) {
            return i;
        }
    }
    return this->size();
}

namespace {

struct Resolver : Ast::Expression::Visitor<void>, Ast::Statement::Visitor<void> {
    Resolver(Locations& l) : locations_{l} {}

    Resolver(const Resolver&) = delete;
    Resolver(Resolver&&) = delete;

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
    ScopeStack scopes_;
    Locations& locations_;
};

void Resolver::operator()(const Ast::Assign& a)
{
    a.expression->accept(*this);
    for_each_variable(*a.variable,
                      [this](const Ast::Variable& v) { locations_[&v] = scopes_.resolve(v); });
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
    locations_[&v] = scopes_.resolve(v);
}

void Resolver::operator()(const Ast::VariableTuple&)
{
    assert(false && "Shouldn't need to evaluate an Ast::VariableTuple, they're just part of "
                    "assignments and variable declarations");
}

void Resolver::operator()(const Ast::Block& b)
{
    scopes_.push();
    for (const auto& statement : b.statements) {
        statement->accept(*this);
    }
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

    if (!scopes_.empty()) {
        scopes_.top().define(*d.variable);
    }
}

}

void resolve(const Ast::Ast& ast, Locations& l)
{
    Resolver r{l};
    for (const auto& statement : ast) {
        statement->accept(r);
    }
}

