#include "scanner.h"
#include "token.h"
#include "error.h"

#include <vector>
#include <optional>
#include <unordered_map>
#include <utility>
#include <cassert>

namespace {

std::optional<Token::Type> keyword_to_token_type(const std::string_view s)
{
    static const std::unordered_map<std::string_view, Token::Type> map{
        {"and", Token::Type::k_and},       {"class", Token::Type::k_class},
        {"else", Token::Type::k_else},     {"false", Token::Type::k_false},
        {"fun", Token::Type::k_fun},       {"for", Token::Type::k_for},
        {"if", Token::Type::k_if},         {"nil", Token::Type::k_nil},
        {"or", Token::Type::k_or},         {"return", Token::Type::k_return},
        {"super", Token::Type::k_super},   {"this", Token::Type::k_this},
        {"true", Token::Type::k_true},     {"var", Token::Type::k_var},
        {"while", Token::Type::k_while},
    };

    const auto it = map.find(s);

    if (it == map.end()) {
        return {};
    } else {
        return it->second;
    }
}

}

struct ScanData {
    ScanData(std::string_view source, std::function<void(const ScanError&)> report_error)
        : source_{source}, report_error_{report_error}
    {}
    ScanData(const ScanData&) = delete;
    ScanData(ScanData&&) = delete;

    char read() const noexcept;
    char read_next() const noexcept;
    bool is_at_end() const noexcept;
    char advance() noexcept;
    void retreat() noexcept;

    void increment_line_number() noexcept;
    unsigned line_number() const noexcept;

    bool match(char expected) const noexcept;
    bool match_advance(char expected) noexcept;

    void report_error(const ScanError& e) const;
private:
    bool increment_position(int i = 1) noexcept;

    const std::string_view source_;
    const std::function<void(const ScanError&)> report_error_;
    unsigned line_number_ = 1;
    unsigned position_ = 0;
};

char ScanData::read() const noexcept
{
    if (this->is_at_end()) return 0;
    return source_[position_];
}

char ScanData::read_next() const noexcept
{
    if (position_ >= source_.length() - 1) return 0;
    return source_[position_ + 1];
}

bool ScanData::is_at_end() const noexcept
{
    return position_ >= source_.length();
}

char ScanData::advance() noexcept
{
    const auto ch = this->read();
    this->increment_position();
    return ch;
}

void ScanData::retreat() noexcept
{
    this->increment_position(-1);
}

void ScanData::increment_line_number() noexcept
{
    ++line_number_;
}

unsigned ScanData::line_number() const noexcept
{
    return line_number_;
}

bool ScanData::match(char expected) const noexcept
{
    if (this->is_at_end()) return false;
    if (source_[position_] != expected) return false;

    return true;
}

bool ScanData::match_advance(char expected) noexcept
{
    if (this->is_at_end()) return false;
    if (source_[position_] != expected) return false;

    this->increment_position();
    return true;
}

void ScanData::report_error(const ScanError& e) const
{
    report_error_(e);
}

bool ScanData::increment_position(int i) noexcept
{
    if (position_ + i > source_.length()) return false;
    position_ += i;
    return true;
}

Token scan_string(ScanData& data)
{
    assert(data.match('"') && "Strings must start with \"");

    data.match_advance('"');

    std::string str;

    while (!data.match('"') && !data.is_at_end()) {
        if (data.match('\n')) data.increment_line_number();
        str.push_back(data.advance());
    }

    if (data.is_at_end()) {
        throw ScanError{data.line_number(), "unterminated string"};
    }

    data.match_advance('"');

    return Token{Token::Type::string, "\"" + str + "\"", str, data.line_number()};
}

Token scan_number(ScanData& data)
{
    assert(std::isdigit(data.read()) && "Numbers must start with digits");

    std::string str;
    str.push_back(data.advance());

    while (std::isdigit(data.read())) {
        str.push_back(data.advance());
    }

    // Look for fractional part
    if (data.match('.') && std::isdigit(data.read_next())) {
        str.push_back(data.advance());
        while (std::isdigit(data.read())) {
            str.push_back(data.advance());
        }
    }

    const double literal = std::stod(str);
    return Token{Token::Type::number, std::move(str), literal, data.line_number()};
}

Token scan_identifier(ScanData& data)
{
    assert(std::isalpha(data.read()) && "Identifiers must start with a letter");

    std::string str;
    str.push_back(data.advance());

    while (std::isalnum(data.read())) {
        str.push_back(data.advance());
    }

    const auto type = keyword_to_token_type(str).value_or(Token::Type::identifier);

    const auto literal = [type]() -> std::optional<ObjectReference> {
        switch (type) {
            case Token::Type::k_nil: return nullptr;
            case Token::Type::k_true: return true;
            case Token::Type::k_false: return false;
            default: return {};
        };
    }();

    return Token{type, str, literal, data.line_number()};
}

Token scan_token(ScanData& data)
{
    const auto make_token = [&data](Token::Type tt, std::string str) {
        return Token{tt, std::move(str), {}, data.line_number()};
    };

    if (data.is_at_end()) {
        return Token(Token::Type::eof, "", {}, data.line_number());
    }

    const char ch = data.advance();

    switch (ch) {
        case '(': return make_token(Token::Type::left_paren,  "(");
        case ')': return make_token(Token::Type::right_paren, ")");
        case '{': return make_token(Token::Type::left_brace,  "{");
        case '}': return make_token(Token::Type::right_brace, "}");
        case ',': return make_token(Token::Type::comma,       ",");
        case '.': return make_token(Token::Type::dot,         ".");
        case '+': return make_token(Token::Type::plus,        "+");
        case ';': return make_token(Token::Type::semicolon,   ";");
        case '*': return make_token(Token::Type::star,        "*");
        case '-':
            return data.match_advance('>') ? make_token(Token::Type::send, "->")
                                           : make_token(Token::Type::minus, "-");
        case '!':
            return data.match_advance('=') ? make_token(Token::Type::bang_equal, "!=")
                                           : make_token(Token::Type::bang, "!");
        case '=':
            return data.match_advance('=') ? make_token(Token::Type::equal_equal, "==")
                                           : make_token(Token::Type::equal, "=");
        case '<':
            return data.match_advance('=') ? make_token(Token::Type::less_equal, "<=")
                                           : make_token(Token::Type::less, "<");
        case '>':
            return data.match_advance('=') ? make_token(Token::Type::greater_equal, ">=")
                                           : make_token(Token::Type::greater, ">");
        case '/': {
            if (data.match_advance('/')) {
                // A comment goes until the end of the line
                while (!data.match('\n') && !data.is_at_end()) data.advance();
                break;
            } else {
                return make_token(Token::Type::slash, "/");
            }
        }
        case ' ':
        case '\r':
        case '\t':
            break;
        case '\n':
            data.increment_line_number();
            break;
        case '"':
            data.retreat();
            return scan_string(data);
        default: {
            if (std::isdigit(ch)) {
                data.retreat();
                return scan_number(data);
            } else if (std::isalpha(ch)) {
                data.retreat();
                return scan_identifier(data);
            } else {
                throw ScanError(data.line_number(), "unexpected character");
            }
        }
    }

    // If we get here, it means we couldn't scan a token but encountered something like a comment
    // or a new line. Therefore we try again.
    return scan_token(data);
}

std::vector<Token> scan(const std::string_view source,
                        std::function<void(const ScanError&)> report_error)
{
    ScanData data(source, report_error);
    std::vector<Token> tokens;

    while (true) {
        try {
            const auto token = scan_token(data);

            tokens.push_back(token);
            if (token.type == Token::Type::eof) break;
        }
        catch (const ScanError& e) {
            data.report_error(e);
        }
    }

    return tokens;
}

