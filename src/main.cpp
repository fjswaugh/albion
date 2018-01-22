#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <string_view>

#include "resolver.h"
#include "argagg.hpp"
#include "scanner.h"
#include "interpreter.h"
#include "error.h"
#include "parser.h"
#include "const.h"
#include "environment.h"
#include "ast_printer.h"

struct DebugOptions {
    constexpr DebugOptions(std::uint8_t data) noexcept
        : data_{data}
    {}
    constexpr operator std::uint8_t() const noexcept {
        return data_;
    }

    static const DebugOptions none;
    static const DebugOptions ast;
    static const DebugOptions tokens;
private:
    std::uint8_t data_;
};

const DebugOptions DebugOptions::none   = 0b00000000;
const DebugOptions DebugOptions::ast    = 0b00000001;
const DebugOptions DebugOptions::tokens = 0b00000010;

struct Program {
    Program(DebugOptions debug_options = DebugOptions::none)
        : debug_options_{debug_options}
    {}

    ErrorCode run_file(std::string_view path);
    void run_prompt();
    ErrorCode run(std::string_view source);

    void report(const Error&);
    ErrorCode error() const noexcept {
        return error_code_;
    }
private:
    DebugOptions debug_options_;
    ErrorCode error_code_ = ErrorCode::no_error;
    std::shared_ptr<Environment> environment_ = std::make_shared<Environment>();
    Locations locations_;
};

ErrorCode Program::run_file(std::string_view path)
{
    const auto file = std::ifstream(path.data());
    std::stringstream buffer;
    buffer << file.rdbuf();

    return this->run(buffer.str());
}

void Program::run_prompt()
{
    std::string line;
    while (std::cout << "> " && std::getline(std::cin, line)) {
        this->run(line);
        error_code_ = ErrorCode::no_error;
    }
}

ErrorCode Program::run(std::string_view source)
try {
    const auto tokens = scan(source, [this](const ScanError& e) { this->report(e); });

    if (debug_options_ & DebugOptions::tokens) {
        for (const auto& token : tokens) {
            std::cout << token << '\n';
        }
    }

    auto ast = parse(tokens, [this](const ParseError& e) { this->report(e); });

    if (error_code_ != ErrorCode::no_error) return error_code_;

    if (debug_options_ & DebugOptions::ast) {
        std::cout << to_string(ast) << '\n';
    }

    resolve(ast, locations_);

    interpret(ast, environment_, locations_);

    return error_code_;
}
catch (const RuntimeError& e) {
    this->report(e);
    return error_code_;
}

void Program::report(const Error& e)
{
    std::cerr << e.what() << '\n';
    error_code_ = e.code();
}

int main(int argc, char** argv)
try {
    argagg::parser arg_parser{{
        {"tokens", {"-s", "--scanner-debug"}, "Debug scanner", 0},
        {"ast",    {"-p", "--parser-debug"},  "Debug parser", 0},
    }};

    const auto args = arg_parser.parse(argc, argv);

    const DebugOptions debug_options = static_cast<bool>(args["ast"]) * DebugOptions::ast +
                                       static_cast<bool>(args["tokens"]) * DebugOptions::tokens;

    Program program{debug_options};

    if (args.pos.empty()) {
        program.run_prompt();
    } else if (args.pos.size() == 1) {
        program.run_file(args.pos.front());
    } else {
        std::cerr << "Usage: " << Const::program_name << " [options] [script]\n";
        return static_cast<int>(ErrorCode::bad_program_usage);
    }

    return static_cast<int>(program.error());
}
catch (const ReturnValue& rv) {
    std::cout << to_string(rv.value) << '\n';
    return static_cast<int>(ErrorCode::no_error);
}
catch (const argagg::error& e) {
    std::cerr << e.what() << '\n';
    return static_cast<int>(ErrorCode::bad_program_usage);
}

