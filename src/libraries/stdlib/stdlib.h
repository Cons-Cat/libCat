// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// TODO: Use a numeral concept.
auto alloca(auto size) -> void* {
    return __builtin_alloca(decay_numeral(size));
}

[[deprecated]] constexpr anyint EXIT_SUCCESS = 0;
[[deprecated]] constexpr anyint EXIT_FAILURE = 1;
[[deprecated(
    "nullptr should be used instead of NULL!")]] constexpr anyint NULL = 0;
