#pragma once

// TODO: Use a numeral concept.
auto alloca(auto size) -> void* {
    return __builtin_alloca(decay_numeral(size));
}

constexpr int EXIT_SUCCESS = 0;
constexpr int EXIT_FAILURE = 1;
constexpr auto NULL = nullptr;
