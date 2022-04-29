// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <utility>

constexpr auto meta::is_constant_evaluated() -> cat::bool1 {
    return __builtin_is_constant_evaluated();
}
