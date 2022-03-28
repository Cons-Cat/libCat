// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <numerals>
#include <type_traits>

constexpr auto meta::is_constant_evaluated() -> bool {
    return __builtin_is_constant_evaluated();
}
