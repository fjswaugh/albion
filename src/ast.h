#pragma once

#include <vector>
#include <memory>
#include <optional>
#include "token.h"

namespace Ast {

struct Statement;
struct Expression;

using Ast = std::vector<std::unique_ptr<Statement>>;

// Statements forward declarations
struct Block;
struct ExpressionStatement;
struct If;
struct Return;
struct While;
struct Declaration;

// Expressions forward declarations
struct Assign;
struct Binary;
struct Call;
struct Function;
struct Grouping;
struct Literal;
struct Logical;
struct Tuple;
struct Unary;
struct Variable;
struct VariableTuple;

struct Statement {
    template <typename ReturnType>
    struct Visitor {
        virtual ReturnType operator()(const Block&) const = 0;
        virtual ReturnType operator()(const ExpressionStatement&) const = 0;
        virtual ReturnType operator()(const If&) const = 0;
        virtual ReturnType operator()(const Return&) const = 0;
        virtual ReturnType operator()(const While&) const = 0;
        virtual ReturnType operator()(const Declaration&) const = 0;
        virtual ~Visitor() noexcept {}
    };

    virtual void accept(const Visitor<void>&) const = 0;
    virtual std::string accept(const Visitor<std::string>&) const = 0;
    virtual ~Statement() noexcept {}
};

struct Expression {
    template <typename ReturnType>
    struct Visitor {
        virtual ReturnType operator()(const Assign&) const = 0;
        virtual ReturnType operator()(const Binary&) const = 0;
        virtual ReturnType operator()(const Call&) const = 0;
        virtual ReturnType operator()(const Function&) const = 0;
        virtual ReturnType operator()(const Grouping&) const = 0;
        virtual ReturnType operator()(const Literal&) const = 0;
        virtual ReturnType operator()(const Logical&) const = 0;
        virtual ReturnType operator()(const Tuple&) const = 0;
        virtual ReturnType operator()(const Unary&) const = 0;
        virtual ReturnType operator()(const Variable&) const = 0;
        virtual ReturnType operator()(const VariableTuple&) const = 0;
        virtual ~Visitor() noexcept {}
    };

    virtual void accept(const Visitor<void>&) const = 0;
    virtual ObjectReference accept(const Visitor<ObjectReference>&) const = 0;
    virtual std::string accept(const Visitor<std::string>&) const = 0;
    virtual ~Expression() noexcept {}
};

// Statements -------------------------------------------------------------------------------------

#define ACCEPT_STATEMENT_VISITORS \
    void accept(const Statement::Visitor<void>& v) const override {\
        return v(*this);\
    }\
    std::string accept(const Statement::Visitor<std::string>& v) const override {\
        return v(*this);\
    }

struct If : Statement {
    If(std::unique_ptr<Expression>&& condition, std::unique_ptr<Statement>&& then_branch,
       std::optional<std::unique_ptr<Statement>>&& else_branch)
        : condition{std::move(condition)}, then_branch{std::move(then_branch)},
          else_branch{std::move(else_branch)}
    {}

    ACCEPT_STATEMENT_VISITORS

    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> then_branch;
    std::optional<std::unique_ptr<Statement>> else_branch;
};

struct Block : Statement {
    Block(std::vector<std::unique_ptr<Statement>> s = {})
        : statements{std::move(s)}
    {}
    template <typename... StatementPtrs>
    Block(StatementPtrs&&... s)
    {
        (void)std::initializer_list<int>{(statements.push_back(std::move(s)), 0)...};
    }

    ACCEPT_STATEMENT_VISITORS

    std::vector<std::unique_ptr<Statement>> statements;
};

struct ExpressionStatement : Statement {
    ExpressionStatement(std::optional<std::unique_ptr<Expression>>&& e = {})
        : expression{std::move(e)}
    {}

    ACCEPT_STATEMENT_VISITORS

    std::optional<std::unique_ptr<Expression>> expression;
};

struct Return : Statement {
    Return(Token keyword, std::optional<std::unique_ptr<Expression>>&& expression = {})
        : keyword{std::move(keyword)}, expression{std::move(expression)}
    {}

    ACCEPT_STATEMENT_VISITORS

    Token keyword;
    std::optional<std::unique_ptr<Expression>> expression;
};

struct While : Statement {
    While(std::unique_ptr<Expression>&& e, std::unique_ptr<Statement>&& s)
        : condition{std::move(e)}, body{std::move(s)}
    {}

    ACCEPT_STATEMENT_VISITORS

    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> body;
};

struct Declaration : Statement {
    Declaration(std::unique_ptr<VariableTuple> v, Token token,
                std::optional<std::unique_ptr<Expression>>&& e)
        : variable{std::move(v)}, token{std::move(token)}, initializer{std::move(e)}
    {}

    ACCEPT_STATEMENT_VISITORS

    std::unique_ptr<VariableTuple> variable;
    Token token;
    std::optional<std::unique_ptr<Expression>> initializer;
};

// Expressions ------------------------------------------------------------------------------------

#define ACCEPT_EXPRESSION_VISITORS \
    void accept(const Expression::Visitor<void>& v) const override {\
        return v(*this);\
    }\
    ObjectReference accept(const Expression::Visitor<ObjectReference>& v) const override {\
        return v(*this);\
    }\
    std::string accept(const Expression::Visitor<std::string>& v) const override {\
        return v(*this);\
    }

struct Variable : Expression {
    Variable(Token t)
        : name{std::move(t)}
    {}

    ACCEPT_EXPRESSION_VISITORS

    Token name;
};

struct VariableTuple : Expression {
    template <typename T,
              typename = std::enable_if_t<std::is_same_v<std::decay_t<T>, VariableTuple> == false>>
    VariableTuple(T&& contents)
        : contents{std::forward<T>(contents)}
    {}

    ACCEPT_EXPRESSION_VISITORS

    std::variant<Variable, std::vector<VariableTuple>> contents;
};

// Function expressions need to be copyable, since they are represented in the ast, and the
// environment (within objects)
struct Function : Expression {
    Function(FunctionInput<std::shared_ptr<VariableTuple>>&& input,
                       std::shared_ptr<Block>&& body)
        : input{std::move(input)}, body{std::move(body)}
    {}

    ACCEPT_EXPRESSION_VISITORS

    FunctionInput<std::shared_ptr<VariableTuple>> input;
    std::shared_ptr<Block> body;
};

struct Assign : Expression {
    Assign(std::unique_ptr<VariableTuple>&& variable, Token token,
           std::unique_ptr<Expression>&& value)
        : variable{std::move(variable)}, token{std::move(token)}, expression{std::move(value)}
    {}

    ACCEPT_EXPRESSION_VISITORS

    std::unique_ptr<VariableTuple> variable;
    Token token;
    std::unique_ptr<Expression> expression;
};

struct Binary : Expression {
    Binary(std::unique_ptr<Expression>&& left, Token op, std::unique_ptr<Expression>&& right)
        : left{std::move(left)}, op{std::move(op)}, right{std::move(right)}
    {}

    ACCEPT_EXPRESSION_VISITORS

    std::unique_ptr<Expression> left;
    Token op;
    std::unique_ptr<Expression> right;
};

struct Tuple : Expression {
    Tuple(std::vector<std::unique_ptr<Expression>> elements)
        : elements{std::move(elements)}
    {}
    template <typename... ElemPtrs>
    Tuple(ElemPtrs&&... elems)
    {
        (void)std::initializer_list<int>{(elements.push_back(std::move(elems)), 0)...};
    }

    ACCEPT_EXPRESSION_VISITORS

    std::vector<std::unique_ptr<Expression>> elements;
};

struct Call : Expression {
    Call(std::unique_ptr<Expression>&& callee, Token token,
         FunctionInput<std::unique_ptr<Expression>>&& input)
        : callee{std::move(callee)}, token{std::move(token)}, input{std::move(input)}
    {}

    ACCEPT_EXPRESSION_VISITORS

    std::unique_ptr<Expression> callee;
    Token token;
    FunctionInput<std::unique_ptr<Expression>> input;
};

struct Grouping : Expression {
    Grouping(std::unique_ptr<Expression>&& e)
        : expression{std::move(e)}
    {}

    ACCEPT_EXPRESSION_VISITORS

    std::unique_ptr<Expression> expression;
};

struct Literal : Expression {
    Literal(const ObjectReference& o)
        : value{o}
    {}
    Literal(ObjectReference&& o)
        : value{std::move(o)}
    {}

    ACCEPT_EXPRESSION_VISITORS

    ObjectReference value;
};

struct Logical : Expression {
    Logical(std::unique_ptr<Expression>&& left, Token op, std::unique_ptr<Expression>&& right)
        : left{std::move(left)}, op{std::move(op)}, right{std::move(right)}
    {}

    ACCEPT_EXPRESSION_VISITORS

    std::unique_ptr<Expression> left;
    Token op;
    std::unique_ptr<Expression> right;
};

struct Unary : Expression {
    Unary(Token op, std::unique_ptr<Expression>&& right)
        : op{std::move(op)}, right{std::move(right)}
    {}

    ACCEPT_EXPRESSION_VISITORS

    Token op;
    std::unique_ptr<Expression> right;
};

}

