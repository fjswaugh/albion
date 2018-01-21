#include "parser.h"
#include "error.h"
#include "ast.h"
#include "function.h"
#include "token.h"
#include "object.h"
#include "function_input.h"

#include <cassert>
#include <memory>
#include <vector>

using namespace std::literals;

namespace {

struct ParseData {
    ParseData(const std::vector<Token>& tokens,
              std::function<void(const ParseError&)> report_error);
    ParseData(const ParseData&) = delete;
    ParseData(ParseData&&) = delete;

    const Token& read() const noexcept;
    bool is_at_end() const noexcept;
    const Token& advance() noexcept;

    template <typename... TokenType> bool match(TokenType... types) const noexcept;
    template <typename... TokenType> bool match_advance(TokenType... types) noexcept;
    const Token& expect(Token::Type type, const std::string_view error_message);

    void save_position() noexcept { saved_position_ = position_; }
    void return_to_saved_position() noexcept { position_ = saved_position_; }

    void synchronize() noexcept;

    void report_error(const ParseError& e) const;
private:
    bool increment_position(int i = 1) noexcept;

    const std::vector<Token>& tokens_;
    const std::function<void(const ParseError&)> report_error_;

    std::size_t position_ = 0;
    std::size_t saved_position_ = 0;
};

ParseData::ParseData(const std::vector<Token>& tokens,
                     std::function<void(const ParseError&)> report_error)
    : tokens_{tokens}, report_error_{report_error}
{
    assert(!tokens_.empty() && tokens_.back().type == Token::Type::eof &&
           "Parse data must end with eof token");
}

const Token& ParseData::read() const noexcept {
    return tokens_[position_];
}

bool ParseData::is_at_end() const noexcept {
    return position_ == tokens_.size() - 1;
}

bool ParseData::increment_position(int i) noexcept {
    if (position_ >= tokens_.size() - i) return false;
    if (i < 0 && position_ < static_cast<std::size_t>(-i)) return false;

    position_ += i;
    return true;
}

const Token& ParseData::advance() noexcept {
    const auto& token = this->read();
    this->increment_position();
    return token;
}

template <typename... TokenType>
bool ParseData::match(TokenType... types) const noexcept
{
    const auto& token = this->read();
    return ((token.type == types) || ...);
}

// Matches next token against token types, and advances if there is a match
template <typename... TokenType>
bool ParseData::match_advance(TokenType... types) noexcept
{
    if (this->match(types...)) {
        this->increment_position();
        return true;
    } else {
        return false;
    }
}

const Token& ParseData::expect(Token::Type type, const std::string_view error_message)
{
    const auto& token = this->read();

    if (token.type == type) {
        this->increment_position();
        return token;
    }

    throw ParseError(token, std::string(error_message));
}

void ParseData::synchronize() noexcept
{
    while (true) {
        if (this->match(Token::Type::semicolon)) {
            this->increment_position();
            return;
        }

        if (!this->increment_position()) return;

        if (this->match(Token::Type::k_class, Token::Type::k_fun, Token::Type::k_var,
                        Token::Type::k_for, Token::Type::k_if, Token::Type::k_while,
                        Token::Type::k_return)) {
            return;
        }
    }
}

void ParseData::report_error(const ParseError& e) const {
    report_error_(e);
}

std::unique_ptr<Ast::VariableTuple> parse_variable_tuple(ParseData&);
std::unique_ptr<Ast::Function> parse_function(ParseData&);
std::unique_ptr<Ast::Expression> parse_primary(ParseData&);
std::unique_ptr<Ast::Expression> parse_unary_call(ParseData& data);
std::unique_ptr<Ast::Expression> parse_n_ary_call(ParseData& data);
std::unique_ptr<Ast::Expression> parse_unary(ParseData&);
std::unique_ptr<Ast::Expression> parse_factor(ParseData&);
std::unique_ptr<Ast::Expression> parse_term(ParseData&);
std::unique_ptr<Ast::Expression> parse_equality(ParseData&);
std::unique_ptr<Ast::Expression> parse_and(ParseData&);
std::unique_ptr<Ast::Expression> parse_or(ParseData&);
std::unique_ptr<Ast::Expression> parse_tuple(ParseData&);
std::unique_ptr<Ast::Expression> parse_assignment(ParseData&);
std::unique_ptr<Ast::Expression> parse_expression(ParseData&);
std::unique_ptr<Ast::Statement> parse_expression_statement(ParseData&);
std::unique_ptr<Ast::Block> parse_block(ParseData&);
std::unique_ptr<Ast::Statement> parse_if_statement(ParseData&);
std::unique_ptr<Ast::Statement> parse_while_statement(ParseData&);
std::unique_ptr<Ast::Statement> parse_for_statement(ParseData&);
std::unique_ptr<Ast::Return> parse_return_statement(ParseData&);
std::unique_ptr<Ast::Statement> parse_statement(ParseData&);
std::unique_ptr<Ast::Statement> parse_var_declaration(ParseData&);
std::unique_ptr<Ast::Statement> parse_declaration(ParseData&);

std::unique_ptr<Ast::VariableTuple> parse_variable_tuple(ParseData& data)
{
    std::unique_ptr<Ast::VariableTuple> result = nullptr;

    const bool leading_comma = data.match_advance(Token::Type::comma);

    if (data.match_advance(Token::Type::left_paren)) {
        result = parse_variable_tuple(data);
        data.expect(Token::Type::right_paren, "expect ')'");
    } else if (data.match(Token::Type::identifier)) {
        result = std::make_unique<Ast::VariableTuple>(data.advance());
    } else {
        throw ParseError(data.read(), "expected identifier(s)");
    }

    if (data.match_advance(Token::Type::comma)) {
        std::vector<Ast::VariableTuple> vec;
        vec.push_back(*result);

        do {
            if (data.match_advance(Token::Type::left_paren)) {
                vec.push_back(*parse_variable_tuple(data));
                data.expect(Token::Type::right_paren, "expect ')'");
            } else if (data.match(Token::Type::identifier)) {
                vec.push_back(Ast::VariableTuple(data.advance()));
            } else {
                throw ParseError(data.read(), "expected identifier(s)");
            }
        } while (data.match_advance(Token::Type::comma));

        return std::make_unique<Ast::VariableTuple>(std::move(vec));
    }

    // Ast::If there was a leading comma, then it should be a tuple, not a variable
    if (leading_comma) {
        std::vector<Ast::VariableTuple> vec;
        vec.push_back(*result);
        return std::make_unique<Ast::VariableTuple>(std::move(vec));
    }

    return result;
}

std::unique_ptr<Ast::Function> parse_function(ParseData& data)
{
    data.expect(Token::Type::k_fun, "expect fun keyword to begin function expression");

    auto input = [&data]() -> FunctionInput<std::shared_ptr<Ast::VariableTuple>> {
        if (!data.match(Token::Type::left_brace)) {
            auto input_0 = parse_variable_tuple(data);
            if (!data.match(Token::Type::left_brace)) {
                auto input_1 = parse_variable_tuple(data);
                return {std::shared_ptr<Ast::VariableTuple>(input_0.release()),
                        std::shared_ptr<Ast::VariableTuple>(input_1.release())};
            }
            return {std::shared_ptr<Ast::VariableTuple>(input_0.release())};
        }

        return {};
    }();

    auto block = std::shared_ptr<Ast::Block>(parse_block(data).release());

    return std::make_unique<Ast::Function>(std::move(input), std::move(block));
}

std::unique_ptr<Ast::Expression> parse_primary(ParseData& data)
{
    if (data.match(Token::Type::k_fun)) return parse_function(data);
    if (data.match_advance(Token::Type::k_false)) return std::make_unique<Ast::Literal>(false);
    if (data.match_advance(Token::Type::k_true)) return std::make_unique<Ast::Literal>(true);
    if (data.match_advance(Token::Type::k_nil)) return std::make_unique<Ast::Literal>(nullptr);

    if (data.match(Token::Type::number, Token::Type::string)) {
        const auto& token = data.advance();
        assert(token.literal && "Token type indicates there should be literal but none present");
        return std::make_unique<Ast::Literal>(*token.literal);
    }

    if (data.match(Token::Type::identifier)) {
        const auto& token = data.advance();
        return std::make_unique<Ast::Variable>(token);
    }

    if (data.match_advance(Token::Type::left_paren)) {
        auto e = parse_expression(data);
        if (!data.match_advance(Token::Type::right_paren)) {
            throw ParseError(data.read(), "expected ')' after expression");
        }
        return std::make_unique<Ast::Grouping>(std::move(e));
    }

    throw ParseError(data.read(), "expect expression");
}

std::unique_ptr<Ast::Expression> parse_unary_call(ParseData& data)
{
    if (!data.match(Token::Type::dot)) return parse_primary(data);

    const auto& token = data.advance();

    std::unique_ptr<Ast::Expression> callee = parse_unary_call(data);

    return std::make_unique<Ast::Call>(std::move(callee), token,
                                  FunctionInput<std::unique_ptr<Ast::Expression>>{});
}

std::unique_ptr<Ast::Expression> parse_n_ary_call(ParseData& data)
{
    std::unique_ptr<Ast::Expression> expression = parse_unary_call(data);

    while (data.match(Token::Type::dot)) {
        const auto& token = data.advance();

        std::unique_ptr<Ast::Expression> callee = parse_unary_call(data);

        if (data.match(Token::Type::left_paren)) {
            std::unique_ptr<Ast::Expression> input_1 = parse_primary(data);
            auto input = FunctionInput{std::move(expression), std::move(input_1)};
            expression = std::make_unique<Ast::Call>(std::move(callee), token, std::move(input));
        } else {
            auto input = FunctionInput{std::move(expression)};
            expression = std::make_unique<Ast::Call>(std::move(callee), token, std::move(input));
        }
    }

    return expression;
}

std::unique_ptr<Ast::Expression> parse_unary(ParseData& data)
{
    if (data.match(Token::Type::bang, Token::Type::minus)) {
        const auto& op = data.advance();
        auto right = parse_unary(data);

        return std::make_unique<Ast::Unary>(op, std::move(right));
    }

    return parse_n_ary_call(data);
}

std::unique_ptr<Ast::Expression> parse_factor(ParseData& data)
{
    auto e = parse_unary(data);

    while (data.match(Token::Type::slash, Token::Type::star)) {
        const auto& op = data.advance();
        auto right = parse_unary(data);

        e = std::make_unique<Ast::Binary>(std::move(e), op, std::move(right));
    }

    return e;
}

std::unique_ptr<Ast::Expression> parse_term(ParseData& data)
{
    auto e = parse_factor(data);

    while (data.match(Token::Type::minus, Token::Type::plus)) {
        const auto& op = data.advance();
        auto right = parse_factor(data);

        e = std::make_unique<Ast::Binary>(std::move(e), op, std::move(right));
    }

    return e;
}

std::unique_ptr<Ast::Expression> parse_comparison(ParseData& data)
{
    auto e = parse_term(data);

    while (data.match(Token::Type::greater, Token::Type::greater_equal, Token::Type::less,
                 Token::Type::less_equal)) {
        const auto& op = data.advance();
        auto right = parse_term(data);

        e = std::make_unique<Ast::Binary>(std::move(e), op, std::move(right));
    }

    return e;
}

std::unique_ptr<Ast::Expression> parse_equality(ParseData& data)
{
    auto e = parse_comparison(data);

    while (data.match(Token::Type::bang_equal, Token::Type::equal_equal)) {
        const auto& bop = data.advance();
        auto right = parse_comparison(data);

        e = std::make_unique<Ast::Binary>(std::move(e), bop, std::move(right));
    }

    return e;
}

std::unique_ptr<Ast::Expression> parse_and(ParseData& data)
{
    auto expression = parse_equality(data);

    while (data.match(Token::Type::k_and)) {
        const Token& op = data.advance();
        auto right = parse_equality(data);
        expression = std::make_unique<Ast::Logical>(std::move(expression), op, std::move(right));
    }

    return expression;
}

std::unique_ptr<Ast::Expression> parse_or(ParseData& data)
{
    auto expression = parse_and(data);

    while (data.match(Token::Type::k_or)) {
        const Token& op = data.advance();
        auto right = parse_and(data);
        expression = std::make_unique<Ast::Logical>(std::move(expression), op, std::move(right));
    }

    return expression;
}

std::unique_ptr<Ast::Expression> parse_tuple(ParseData& data)
{
    const bool leading_comma = data.match_advance(Token::Type::comma);

    auto expression = parse_or(data);

    if (data.match_advance(Token::Type::comma)) {
        std::vector<std::unique_ptr<Ast::Expression>> elements;
        elements.push_back(std::move(expression));

        do {
            elements.push_back(parse_or(data));
        } while (data.match_advance(Token::Type::comma));

        return std::make_unique<Ast::Tuple>(std::move(elements));
    }

    if (leading_comma) {
        std::vector<std::unique_ptr<Ast::Expression>> elements;
        elements.push_back(std::move(expression));
        return std::make_unique<Ast::Tuple>(std::move(elements));
    }

    return expression;
}

std::unique_ptr<Ast::Expression> parse_send_call(ParseData& data)
{
    std::unique_ptr<Ast::Expression> expression = parse_tuple(data);

    while (data.match(Token::Type::send)) {
        const auto& token = data.advance();

        std::unique_ptr<Ast::Expression> callee = parse_tuple(data);

        if (data.match(Token::Type::left_paren)) {
            std::unique_ptr<Ast::Expression> input_1 = parse_primary(data);
            auto input = FunctionInput{std::move(expression), std::move(input_1)};
            expression = std::make_unique<Ast::Call>(std::move(callee), token, std::move(input));
        } else {
            auto input = FunctionInput{std::move(expression)};
            expression = std::make_unique<Ast::Call>(std::move(callee), token, std::move(input));
        }
    }

    return expression;
}


std::unique_ptr<Ast::Expression> parse_assignment(ParseData& data)
{
    data.save_position();

    auto expression = parse_send_call(data);

    if (data.match(Token::Type::equal)) {
        data.return_to_saved_position();

        auto variable_tuple = parse_variable_tuple(data);
        auto token = data.expect(Token::Type::equal, "error parsing assignment");
        auto value = parse_assignment(data);

        return std::make_unique<Ast::Assign>(std::move(variable_tuple), std::move(token),
                                        std::move(value));
    }

    return expression;
}

std::unique_ptr<Ast::Expression> parse_expression(ParseData& data)
{
    return parse_assignment(data);
}

std::unique_ptr<Ast::Statement> parse_expression_statement(ParseData& data)
{
    if (data.match_advance(Token::Type::semicolon)) {
        return std::make_unique<Ast::ExpressionStatement>();
    }

    auto expression = parse_expression(data);
    data.expect(Token::Type::semicolon, "expect ';' after expression");
    return std::make_unique<Ast::ExpressionStatement>(std::move(expression));
}

std::unique_ptr<Ast::Block> parse_block(ParseData& data)
{
    data.expect(Token::Type::left_brace, "expect '{' to start block");

    std::vector<std::unique_ptr<Ast::Statement>> statements;

    while (!data.match(Token::Type::right_brace) && !data.is_at_end()) {
        statements.push_back(parse_declaration(data));
    }

    data.expect(Token::Type::right_brace, "expect '}' after block");

    return std::make_unique<Ast::Block>(std::move(statements));
}

std::unique_ptr<Ast::Statement> parse_if_statement(ParseData& data)
{
    data.expect(Token::Type::left_paren, "expect '(' after 'if'");
    auto condition = parse_expression(data);
    data.expect(Token::Type::right_paren, "expect ')' after if condition");

    auto then_branch = parse_statement(data);
    std::optional<std::unique_ptr<Ast::Statement>> else_branch;  // Default to empty block
    if (data.match_advance(Token::Type::k_else)) else_branch = parse_statement(data);

    return std::make_unique<Ast::If>(std::move(condition), std::move(then_branch),
                                std::move(else_branch));
}

std::unique_ptr<Ast::Statement> parse_while_statement(ParseData& data)
{
    data.expect(Token::Type::left_paren, "expect '(' after 'while'");
    auto condition = parse_expression(data);
    data.expect(Token::Type::right_paren, "expect ')' after condition");

    auto body = parse_statement(data);
    return std::make_unique<Ast::While>(std::move(condition), std::move(body));
}

std::unique_ptr<Ast::Statement> parse_for_statement(ParseData& data)
{
    // Parse loop

    data.expect(Token::Type::left_paren, "expect '(' after 'for'");

    std::unique_ptr<Ast::Statement> initializer = data.match_advance(Token::Type::k_var)
                                                 ? parse_var_declaration(data)
                                                 : parse_expression_statement(data);

    std::optional<std::unique_ptr<Ast::Expression>> condition;
    if (!data.match(Token::Type::semicolon)) {
        condition = parse_expression(data);
    }

    data.expect(Token::Type::semicolon, "expect ';' after loop condition");

    std::optional<std::unique_ptr<Ast::Expression>> increment;
    if (!data.match(Token::Type::right_paren)) {
        increment = parse_expression(data);
    }

    data.expect(Token::Type::right_paren, "expect ')' after for clauses");

    std::unique_ptr<Ast::Statement> body = parse_statement(data);

    // Convert to while loop

    if (increment) {
        body = std::make_unique<Ast::Block>(
            std::move(body), std::make_unique<Ast::ExpressionStatement>(std::move(increment)));
    }

    if (!condition) condition = std::make_unique<Ast::Literal>(true);
    body = std::make_unique<Ast::While>(std::move(*condition), std::move(body));

    body = std::make_unique<Ast::Block>(std::move(initializer), std::move(body));

    return body;
}

std::unique_ptr<Ast::Return> parse_return_statement(ParseData& data)
{
    const Token& keyword = data.advance();

    std::optional<std::unique_ptr<Ast::Expression>> expression;
    if (!data.match(Token::Type::semicolon)) {
        expression = parse_expression(data);
    }

    data.expect(Token::Type::semicolon, "expect ';' after return value");
    return std::make_unique<Ast::Return>(keyword, std::move(expression));
}

std::unique_ptr<Ast::Statement> parse_statement(ParseData& data)
{
    if (data.match_advance(Token::Type::k_for)) return parse_for_statement(data);
    if (data.match_advance(Token::Type::k_if)) return parse_if_statement(data);
    if (data.match(Token::Type::k_return)) return parse_return_statement(data);
    if (data.match_advance(Token::Type::k_while)) return parse_while_statement(data);

    if (data.match(Token::Type::left_brace)) {
        return parse_block(data);
    }

    return parse_expression_statement(data);
}

std::unique_ptr<Ast::Statement> parse_var_declaration(ParseData& data)
{
    auto variable_tuple = parse_variable_tuple(data);

    std::optional<std::unique_ptr<Ast::Expression>> initializer = {};
    if (data.match_advance(Token::Type::equal)) {
        initializer = parse_expression(data);
    }

    auto token = data.expect(Token::Type::semicolon, "expect ';' after variable declaration");

    return std::make_unique<Ast::Declaration>(std::move(variable_tuple), std::move(token),
                                         std::move(initializer));
}

std::unique_ptr<Ast::Statement> parse_declaration(ParseData& data)
{
    if (data.match_advance(Token::Type::k_var)) return parse_var_declaration(data);

    return parse_statement(data);
}

}  // End of anonymous namespace

Ast::Ast parse(const std::vector<Token>& tokens, std::function<void(const ParseError&)> report_error)
{
    ParseData data(tokens, report_error);
    std::vector<std::unique_ptr<Ast::Statement>> statements;

    while (!data.is_at_end()) {
        try {
            statements.push_back(parse_declaration(data));
        }
        catch (const ParseError& e) {
            data.report_error(e);
            data.synchronize();
        }
    }

    return statements;
}

