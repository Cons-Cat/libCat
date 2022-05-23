// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/utility>

constexpr auto cat::is_constant_evaluated() -> cat::bool1 {
    return __builtin_is_constant_evaluated();
}
