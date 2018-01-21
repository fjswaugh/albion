#pragma once

#include "token.h"

#include <string_view>
#include <string>
#include <iostream>

enum class ErrorCode : int {
    no_error = 0,
    bad_program_usage = 1,
    scan_error = 2,
    parse_error = 3,
    runtime_error = 4,
};

struct Error : std::exception {
    virtual ErrorCode code() const noexcept = 0;
};

struct ScanError : Error {
    explicit ScanError(std::size_t line_number, std::string message)
        : message_{"[line " + std::to_string(line_number) + "] Scan error: " + std::move(message)}
    {}

    ~ScanError() override {}

    const char* what() const noexcept override {
        return message_.c_str();
    }

    ErrorCode code() const noexcept override {
        return ErrorCode::scan_error;
    }
private:
    std::string message_;
};

struct ParseError : Error {
    explicit ParseError(const Token& token, std::string message)
        : message_{"[line " + std::to_string(token.line) + "] Parse error: " + std::move(message)}
    {}

    const char* what() const noexcept override {
        return message_.c_str();
    }

    void concatenate(const ParseError& pe) {
        message_ += "\n";
        message_ += pe.what();
    }

    ErrorCode code() const noexcept override {
        return ErrorCode::parse_error;
    }
private:
    std::string message_;
};

struct RuntimeError : Error {
    explicit RuntimeError(const Token& token, std::string message)
        : message_{"[line " + std::to_string(token.line) +
                   "] Runtime error: " + std::move(message)}
    {}

    const char* what() const noexcept override
    {
        return message_.c_str();
    }

    ErrorCode code() const noexcept override
    {
        return ErrorCode::runtime_error;
    }

private:
    std::string message_;
};

