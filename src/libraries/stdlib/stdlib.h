// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// TODO: Use a numeral concept.
auto alloca(auto size) -> void* {
    return __builtin_alloca(decay_numeral(size));
}

[[deprecated]] constexpr i4 EXIT_SUCCESS = 0;
[[deprecated]] constexpr i4 EXIT_FAILURE = 1;
[[deprecated("nullptr should be used instead of NULL!")]] constexpr i4 NULL = 0;
