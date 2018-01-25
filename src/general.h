#pragma once

#include <fstream>
#include <string>
#include <sstream>
#include <string_view>

template <typename... F>
inline auto combine(F... f) {
    struct S : F... {
        using F::operator()...;
    };
    return S{f...};
}

inline std::string read_file(std::string_view filepath)
{
    const auto file = std::ifstream{filepath.data()};
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

