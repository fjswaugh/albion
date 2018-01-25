#include "ast_printer.h"
#include "ast.h"

#include <iostream>

namespace {

struct Printer : Ast::Expression::Visitor<std::string>, Ast::Statement::Visitor<std::string> {
    std::string operator()(const Ast::Assign& a) override {
        std::string s;
        s += "(assign ";
        s += a.variable->accept(*this);
        s += a.expression->accept(*this);
        s += ") ";
        return s;
    }
    std::string operator()(const Ast::Binary& b) override {
        std::string s;
        s += "(" + b.op.lexeme + " ";
        s += b.left->accept(*this);
        s += b.right->accept(*this);
        s += ") ";
        return s;
    }
    std::string operator()(const Ast::Call& c) override {
        std::string s;
        s += "(call ";
        s += c.callee->accept(*this);
        for (std::size_t i = 0; i < c.input.size(); ++i) {
            s += c.input[i]->accept(*this);
        }
        s += ") ";
        return s;
    }
    std::string operator()(const Ast::Function& f) override {
        std::string s;
        s += "(fun ";
        for (std::size_t i = 0; i < f.input.size(); ++i) {
            s += f.input[i]->accept(*this);
        }
        s += f.body->accept(*this);
        s += ") ";
        return s;
    }
    std::string operator()(const Ast::Grouping& g) override {
        std::string s;
        s += "(group ";
        s += g.expression->accept(*this);
        s += ") ";
        return s;
    }
    std::string operator()(const Ast::Literal& l) override {
        std::string s;
        const bool is_string = l.value.holds<std::string>();
        if (is_string) s += "\"";
        s += to_string(l.value);
        if (is_string) s += "\"";
        s += " ";
        return s;
    }
    std::string operator()(const Ast::Logical& l) override {
        std::string s;
        s += "(" + l.op.lexeme + " ";
        s += l.left->accept(*this);
        s += l.right->accept(*this);
        s += ") ";
        return s;
    }
    std::string operator()(const Ast::Tuple& t) override {
        std::string s;
        s += "(tuple ";
        for (auto& expression : t.elements) {
            s += expression->accept(*this);
        }
        s += ") ";
        return s;
    }
    std::string operator()(const Ast::Unary& u) override {
        std::string s;
        s += "(" + u.op.lexeme + " ";
        s += u.right->accept(*this);
        s += ") ";
        return s;
    }
    std::string operator()(const Ast::Variable& v) override {
        std::string s;
        s += v.name.lexeme + " ";
        return s;
    }
    std::string operator()(const Ast::VariableTuple& vt) override {
        std::string s;
        const auto f = combine([&s, this](const Ast::Variable& v) {
                                   s += v.accept(*this);
                               },
                               [&s, this](const std::vector<Ast::VariableTuple>& vvt) {
                                   s += "(";
                                   for (const auto& vt : vvt) s += vt.accept(*this);
                                   s += ") ";
                               }
        );

        std::visit(f, vt.contents);
        return s;
    }

    std::string operator()(const Ast::Block& b) override {
        std::string s;
        s += "(block ";
        for (auto& statement : b.statements) {
            s += statement->accept(*this);
        }
        s += ") ";
        return s;
    }
    std::string operator()(const Ast::ExpressionStatement& es) override {
        std::string s;
        s += "(; ";
        if (es.expression) s += (*es.expression)->accept(*this);
        s += ") ";
        return s;
    }
    std::string operator()(const Ast::If& i) override {
        std::string s;
        s += "(if ";
        s += i.condition->accept(*this);
        s += i.then_branch->accept(*this);
        if (i.else_branch) s += (*i.else_branch)->accept(*this);
        s += ") ";
        return s;
    }
    std::string operator()(const Ast::Return& r) override {
        std::string s;
        s += "(return ";
        if (r.expression) {
            s += (*r.expression)->accept(*this);
        }
        s += ") ";
        return s;
    }
    std::string operator()(const Ast::While& w) override {
        std::string s;
        s += "(while ";
        s += w.condition->accept(*this);
        s += w.body->accept(*this);
        s += ") ";
        return s;
    }
    std::string operator()(const Ast::Declaration& d) override {
        std::string s;
        s += "(var ";
        s += d.variable->accept(*this);
        if (d.initializer) s += (*d.initializer)->accept(*this);
        s += ") ";
        return s;
    }
    std::string operator()(const Ast::Import& i) override {
        std::string s;
        s += "(import ";
        s += "\"" + i.filepath + "\" ";
        if (i.variable) {
            s += " as " + (*i.variable)->accept(*this);
        }
        s += ") ";
        return s;
    }
};

}  // Namespace

std::string to_string(const Ast::Ast& ast) {
    std::string s;
    Printer printer;
    for (auto& p : ast) {
        s += p->accept(printer);
    }
    return s;
}

