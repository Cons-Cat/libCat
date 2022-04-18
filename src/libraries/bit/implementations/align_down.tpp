// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <bit>

constexpr auto cat::align_down(intptr const& value, ssize const alignment)
    -> intptr {
    return value & (~(alignment - 1));
}
