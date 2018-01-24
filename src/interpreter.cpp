#include "interpreter.h"
#include "general.h"
#include "object.h"
#include "environment.h"
#include "ast.h"
#include "error.h"
#include "resolver.h"

// Attempt to set a Ast::VariableTuple vt to an ObjectReference o using a SetFunction
// that takes a Ast::Variable and an ObjectReference
template <typename SetFunction>
void set_variable_tuple(SetFunction&& set_function, const Ast::VariableTuple& vt,
                        const ObjectReference& o, const Token& token)
{
    const auto f = combine(
        [&set_function, &o](const Ast::Variable& v) {
            set_function(v, o);
        },
        [&set_function, &o, &token](const std::vector<Ast::VariableTuple>& vvt) {
            if (!o.holds<Tuple>()) throw RuntimeError(token, "can only decompose tuples");

            const auto tuple = o.get<Tuple>();
            if (tuple.size() > vvt.size()) {
                throw RuntimeError(token, "too many arguments to bind");
            }

            for (std::size_t i = 0; i < tuple.size(); ++i) {
                set_variable_tuple(set_function, vvt[i], tuple[i], token);
            }
            for (std::size_t i = tuple.size(); i < vvt.size(); ++i) {
                set_variable_tuple(set_function, vvt[i], nullptr, token);
            }
        }
    );
    std::visit(f, vt.contents);
}

void define_variable_tuple(const Ast::VariableTuple& vt, Environment& e)
{
    const auto f = combine(
        [&e](const Ast::Variable& v) {
            e.define(v.name.lexeme, nullptr);
        },
        [&e](const std::vector<Ast::VariableTuple>& vvt) {
            for (const auto& vt : vvt) define_variable_tuple(vt, e);
        }
    );
    std::visit(f, vt.contents);
}

struct Interpreter : Ast::Expression::Visitor<ObjectReference>, Ast::Statement::Visitor<void> {
    Interpreter(const Locations& locations, std::shared_ptr<Environment> e,
                std::shared_ptr<Environment> ge)
        : locations_{locations}, environment_{std::move(e)}, global_environment_{std::move(ge)}
    {}

    Interpreter(const Interpreter&) = delete;
    Interpreter(Interpreter&&) = delete;

    ObjectReference call(const Function&, const FunctionInput<ObjectReference>&, const Token&);

    ObjectReference operator()(const Ast::Assign&) override;
    ObjectReference operator()(const Ast::Binary&) override;
    ObjectReference operator()(const Ast::Call&) override ;
    ObjectReference operator()(const Ast::Function&) override;
    ObjectReference operator()(const Ast::Grouping&) override;
    ObjectReference operator()(const Ast::Literal&) override;
    ObjectReference operator()(const Ast::Logical&) override;
    ObjectReference operator()(const Ast::Tuple&) override ;
    ObjectReference operator()(const Ast::Unary&) override;
    ObjectReference operator()(const Ast::Variable&) override;
    ObjectReference operator()(const Ast::VariableTuple&) override;

    void operator()(const Ast::Block&) override;
    void operator()(const Ast::ExpressionStatement&) override;
    void operator()(const Ast::If&) override;
    void operator()(const Ast::Return&) override;
    void operator()(const Ast::While&) override;
    void operator()(const Ast::Declaration&) override;

private:
    const Locations& locations_;
    std::shared_ptr<Environment> environment_;
    std::shared_ptr<Environment> global_environment_;
};

ObjectReference Interpreter::operator()(const Ast::Assign& a)
{
    auto value = a.expression->accept(*this);

    const auto set_function = [this](const Ast::Variable& v, const ObjectReference& o) {
        if (const auto location = locations_.find(&v);
            location != locations_.end())
        {
            const auto level = location->second;
            this->environment_->assign_at(v.name, o, level);
        } else {
            this->global_environment_->assign(v.name, o);
        }
    };
    set_variable_tuple(set_function, *a.variable, value, a.token);
    return value;
}

ObjectReference Interpreter::operator()(const Ast::Binary& b)
try {
    auto left = b.left->accept(*this);
    auto right = b.right->accept(*this);

    switch (b.op.type) {
        case Token::Type::minus:
            return left.get<double>() - right.get<double>();
        case Token::Type::slash:
            return left.get<double>() / right.get<double>();
        case Token::Type::star:
            return left.get<double>() * right.get<double>();
        case Token::Type::plus: {
            if (left.holds<double>() && right.holds<double>()) {
                return left.get<double>() + right.get<double>();
            }
            if (left.holds<std::string>() && right.holds<std::string>()) {
                return left.get<std::string>() + right.get<std::string>();
            }
            throw RuntimeError(b.op, "bad operand type");
        }
        case Token::Type::greater:
            return left.get<double>() > right.get<double>();
        case Token::Type::greater_equal:
            return left.get<double>() >= right.get<double>();
        case Token::Type::less:
            return left.get<double>() < right.get<double>();
        case Token::Type::less_equal:
            return left.get<double>() <= right.get<double>();
        case Token::Type::bang_equal:
            return left != right;
        case Token::Type::equal_equal:
            return left == right;
        default:
            throw RuntimeError(b.op, "bad operator type");
    };
}
catch (const ObjectReference::Bad_access&) {
    throw RuntimeError(b.op, "bad operand type");
}

ObjectReference Interpreter::operator()(const Ast::Call& c) {
    ObjectReference callee = c.callee->accept(*this);

    FunctionInput<ObjectReference> input = [this, &c] {
        switch (c.input.size()) {
        case 0:
            return FunctionInput<ObjectReference>();
        case 1:
            return FunctionInput<ObjectReference>(c.input.at(0)->accept(*this));
        default:
            return FunctionInput<ObjectReference>(c.input.at(0)->accept(*this),
                                                  c.input.at(1)->accept(*this));
        };
    }();

    const auto visitor = combine(
        [&](const Function& f) -> ObjectReference {
            return this->call(f, input, c.token);
        },
        [&](const BuiltInFunction& f) -> ObjectReference {
            return f.call(input, c.token);
        },
        [&](auto) -> ObjectReference {
            throw RuntimeError(c.token, "can only call functions");
        }
    );

    return callee.visit(visitor);
}

ObjectReference Interpreter::operator()(const Ast::Function& f)
{
    return Function(f, environment_);
}

ObjectReference Interpreter::operator()(const Ast::Grouping& g)
{
    return g.expression->accept(*this);
};

ObjectReference Interpreter::operator()(const Ast::Literal& l)
{
    return l.value;
};

ObjectReference Interpreter::operator()(const Ast::Logical& l)
{
    ObjectReference left = l.left->accept(*this);

    if (l.op.type == Token::Type::k_or) {
        if (is_truthy(left)) return true;
    } else {
        if (!is_truthy(left)) return false;
    } 

    return l.right->accept(*this);
};

ObjectReference Interpreter::operator()(const Ast::Tuple& t) {
    Tuple tuple;
    for (auto& expression : t.elements) {
        tuple.push_back(expression->accept(*this));
    }
    return tuple;
};

ObjectReference Interpreter::operator()(const Ast::Unary& u)
try {
    auto right = u.right->accept(*this);

    if (u.op.type == Token::Type::minus) {
        return -right.get<double>();
    }

    if (u.op.type == Token::Type::bang) {
        return !is_truthy(right);
    }

    throw RuntimeError(u.op, "bad operator type");
}
catch (const ObjectReference::Bad_access&) {
    throw RuntimeError(u.op, "bad operand type");
}

ObjectReference Interpreter::operator()(const Ast::Variable& v)
{
    if (const auto location = locations_.find(&v);
        location != locations_.end())
    {
        return environment_->get_at(v.name, location->second);
    } else {
        return global_environment_->get(v.name);
    }
}

ObjectReference Interpreter::operator()(const Ast::VariableTuple&)
{
    assert(false && "Shouldn't need to evaluate an Ast::VariableTuple, they're just part of "
                    "assignments and variable declarations");
    return nullptr;
}


void Interpreter::operator()(const Ast::Block& b)
{
    auto new_environment = std::make_shared<Environment>(environment_);
    Interpreter new_interpreter{locations_, new_environment, global_environment_};

    for (auto& statement : b.statements) {
        statement->accept(new_interpreter);
    }
}

void Interpreter::operator()(const Ast::ExpressionStatement& es)
{
    if (es.expression) (*es.expression)->accept(*this);
}

void Interpreter::operator()(const Ast::If& i)
{
    if (is_truthy(i.condition->accept(*this))) {
        i.then_branch->accept(*this);
    } else {
        if (i.else_branch) (*i.else_branch)->accept(*this);
    }
}

void Interpreter::operator()(const Ast::Return& r)
{
    // Default return value is nil
    ObjectReference value = nullptr;

    if (r.expression) {
        value = (*r.expression)->accept(*this);
    }

    throw ReturnValue{std::move(value)};
}

void Interpreter::operator()(const Ast::While& w)
{
    while (is_truthy(w.condition->accept(*this))) {
        w.body->accept(*this);
    }
}

void Interpreter::operator()(const Ast::Declaration& d)
{
    if (!d.initializer) {
        define_variable_tuple(*d.variable, *environment_);
        return;
    }

    ObjectReference value = (*d.initializer)->accept(*this);

    const auto set_function = [this](const Ast::Variable& v, const ObjectReference& o) {
        environment_->define(v.name.lexeme, o);
    };
    set_variable_tuple(set_function, *d.variable, value, d.token);
}

ObjectReference Interpreter::call(const Function& f, const FunctionInput<ObjectReference>& input,
                                  const Token& token)
{
    if (input.size() > f.expression().input.size()) {
        auto expects = std::to_string(f.expression().input.size());
        auto received = std::to_string(input.size());
        auto message = "function expects " + expects + " inputs, but receieved " + received;
        throw RuntimeError(token, std::move(message));
    }

    auto new_environment = std::make_shared<Environment>(f.closure());
    Interpreter new_interpreter{locations_, new_environment, global_environment_};

    const auto set_function = [&new_environment](const Ast::Variable& v,
                                                 const ObjectReference& o) {
        new_environment->define(v.name.lexeme, o);
    };

    for (std::size_t i = 0; i < input.size(); ++i) {
        set_variable_tuple(set_function, *f.expression().input[i], input[i], token);
    }
    for (std::size_t i = input.size(); i < f.expression().input.size(); ++i) {
        define_variable_tuple(*f.expression().input[i], *new_environment);
    }

    try {
        f.expression().body->accept(new_interpreter);
    } catch (const ReturnValue& rv) {
        return rv.value;
    }
    return nullptr;
}

void interpret(const Ast::Ast& ast, const Locations& locations,
               std::shared_ptr<Environment> environment,
               std::shared_ptr<Environment> global_environment)
{
    Interpreter i{locations, std::move(environment), std::move(global_environment)};
    for (auto& statement : ast) {
        statement->accept(i);
    }
}

