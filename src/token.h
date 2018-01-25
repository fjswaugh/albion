#pragma once

#include "object.h"

#include <string>
#include <sstream>
#include <cassert>
#include <algorithm>
#include <variant>
#include <optional>

struct Token {
    enum class Type {
        // Single character tokens
        left_paren, right_paren, left_brace, right_brace,
        comma, dot, minus, plus, semicolon, slash, star,

        // One or two character tokens
        bang, bang_equal, equal, equal_equal,
        greater, greater_equal, less, less_equal,
        send,

        // Literals
        identifier, string, number,

        // Keywords
        k_and, k_class, k_else, k_false, k_fun, k_for, k_if, k_nil, k_or,
        k_return, k_super, k_this, k_true, k_var, k_while, k_import, k_as,

        eof
    };

    Token(Type t, std::string lex, std::optional<ObjectReference> lit, unsigned l)
        : type{t}, lexeme{std::move(lex)}, literal{std::move(lit)}, line{l}
    {}

    Type type;
    std::string lexeme;
    std::optional<ObjectReference> literal;
    unsigned line;
};

inline std::string to_string(const Token::Type tt) {
    switch (tt) {
        case Token::Type::left_paren: return "left paren";
        case Token::Type::right_paren: return "right_paren";
        case Token::Type::left_brace: return "left_brace";
        case Token::Type::right_brace: return "right_brace";
        case Token::Type::comma: return "comma";
        case Token::Type::dot: return "dot";
        case Token::Type::minus: return "minus";
        case Token::Type::plus: return "plus";
        case Token::Type::semicolon: return "semicolon";
        case Token::Type::slash: return "slash";
        case Token::Type::star: return "star";
        case Token::Type::bang: return "bang";
        case Token::Type::bang_equal: return "bang_equal";
        case Token::Type::equal: return "equal";
        case Token::Type::equal_equal: return "equal_equal";
        case Token::Type::greater: return "greater";
        case Token::Type::greater_equal: return "greater_equal";
        case Token::Type::less: return "less";
        case Token::Type::less_equal: return "less_equal";
        case Token::Type::send: return "send";
        case Token::Type::identifier: return "identifier";
        case Token::Type::string: return "string";
        case Token::Type::number: return "number";
        case Token::Type::k_and: return "k_and";
        case Token::Type::k_class: return "k_class";
        case Token::Type::k_else: return "k_else";
        case Token::Type::k_false: return "k_false";
        case Token::Type::k_fun: return "k_fun";
        case Token::Type::k_for: return "k_for";
        case Token::Type::k_if: return "k_if";
        case Token::Type::k_nil: return "k_nil";
        case Token::Type::k_or: return "k_or";
        case Token::Type::k_return: return "k_return";
        case Token::Type::k_super: return "k_super";
        case Token::Type::k_this: return "k_this";
        case Token::Type::k_true: return "k_true";
        case Token::Type::k_var: return "k_var";
        case Token::Type::k_while: return "k_while";
        case Token::Type::k_import: return "k_import";
        case Token::Type::k_as: return "k_as";
        case Token::Type::eof: return "eof";
        default: assert(false); return "";
    }
}

inline std::string to_string(const Token& token) {
    if (token.type == Token::Type::eof) return "eof";

    const auto token_type_str = to_string(token.type);
    const auto spaces = std::string(13 - token_type_str.size(), ' ');

    auto str = token_type_str + spaces + " -- " + token.lexeme;

    if (token.literal) {
        str += std::string(std::max(5 - static_cast<int>(token.lexeme.size()), 0), ' ') + " -- " +
               to_string(*token.literal);
    }
    return str;
}

inline std::ostream& operator<<(std::ostream& os, const Token& t)
{
    return os << to_string(t);
}

