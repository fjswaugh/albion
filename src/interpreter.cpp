#include "interpreter.h"
#include "general.h"
#include "object.h"
#include "environment.h"
#include "ast.h"
#include "error.h"

namespace {

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

ObjectReference call(const Function&, const FunctionInput<ObjectReference>&, const Token&);

struct Interpreter : Ast::Expression::Visitor<ObjectReference>, Ast::Statement::Visitor<void> {
    Interpreter(std::shared_ptr<Environment> e)
        : environment{e}
    {}

    Interpreter(const Interpreter&) = delete;
    Interpreter(Interpreter&&) = delete;

    std::shared_ptr<Environment> environment;

    ObjectReference operator()(const Ast::Assign& a) const override
    {
        auto value = a.expression->accept(*this);

        const auto set_function = [this](const Ast::Variable& v, const ObjectReference& o) {
            this->environment->assign(v.name, o);
        };
        set_variable_tuple(set_function, *a.variable, value, a.token);
        return value;
    }

    ObjectReference operator()(const Ast::Binary& b) const override
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

    ObjectReference operator()(const Ast::Call& c) const override {
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
                return call(f, input, c.token);
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

    ObjectReference operator()(const Ast::Function& f) const override
    {
        return Function(f, environment);
    }

    ObjectReference operator()(const Ast::Grouping& g) const override
    {
        return g.expression->accept(*this);
    };

    ObjectReference operator()(const Ast::Literal& l) const override { return l.value; };

    ObjectReference operator()(const Ast::Logical& l) const override
    {
        ObjectReference left = l.left->accept(*this);

        if (l.op.type == Token::Type::k_or) {
            if (is_truthy(left)) return true;
        } else {
            if (!is_truthy(left)) return false;
        } 

        return l.right->accept(*this);
    };

    ObjectReference operator()(const Ast::Tuple& t) const override {
        Tuple tuple;
        for (auto& expression : t.elements) {
            tuple.push_back(expression->accept(*this));
        }
        return tuple;
    };

    ObjectReference operator()(const Ast::Unary& u) const override
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

    ObjectReference operator()(const Ast::Variable& v) const override
    {
        return environment->get(v.name);
    }

    ObjectReference operator()(const Ast::VariableTuple&) const override
    {
        assert(false &&
               "Shouldn't need to evaluate a Ast::VariableTuple, they're only assigned to");
        return nullptr;
    }


    void operator()(const Ast::Block& b) const override
    {
        auto new_environment = std::make_shared<Environment>(environment);
        Interpreter new_interpreter{new_environment};

        for (auto& statement : b.statements) {
            statement->accept(new_interpreter);
        }
    }

    void operator()(const Ast::ExpressionStatement& es) const override
    {
        if (es.expression) (*es.expression)->accept(*this);
    }

    void operator()(const Ast::If& i) const override
    {
        if (is_truthy(i.condition->accept(*this))) {
            i.then_branch->accept(*this);
        } else {
            if (i.else_branch) (*i.else_branch)->accept(*this);
        }
    }

    void operator()(const Ast::Return& r) const override
    {
        // Default return value is nil
        ObjectReference value = nullptr;

        if (r.expression) {
            value = (*r.expression)->accept(*this);
        }

        throw Return_value{std::move(value)};
    }

    void operator()(const Ast::While& w) const override
    {
        while (is_truthy(w.condition->accept(*this))) {
            w.body->accept(*this);
        }
    }

    void operator()(const Ast::Declaration& d) const override
    {
        if (!d.initializer) {
            define_variable_tuple(*d.variable, *environment);
            return;
        }

        ObjectReference value = (*d.initializer)->accept(*this);

        const auto set_function = [this](const Ast::Variable& v, const ObjectReference& o) {
            environment->define(v.name.lexeme, o);
        };
        set_variable_tuple(set_function, *d.variable, value, d.token);
    }
};

ObjectReference call(const Function& f, const FunctionInput<ObjectReference>& input,
                     const Token& token)
{
    if (input.size() > f.expression().input.size()) {
        auto expects = std::to_string(f.expression().input.size());
        auto received = std::to_string(input.size());
        auto message = "function expects " + expects + " inputs, but receieved " + received;
        throw RuntimeError(token, std::move(message));
    }

    auto new_environment = std::make_shared<Environment>(f.closure());
    Interpreter new_interpreter{new_environment};

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
    } catch (const Return_value& rv) {
        return rv.value;
    }
    return nullptr;
}

}  // Namespace

void interpret(const Ast::Ast& ast, std::shared_ptr<Environment> environment)
{
    Interpreter i{std::move(environment)};
    for (auto& statement : ast) {
        statement->accept(i);
    }
}

