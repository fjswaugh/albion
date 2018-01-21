#pragma once

template <typename... F>
inline auto combine(F... f) {
    struct S : F... {
        using F::operator()...;
    };
    return S{f...};
}

