// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <bit>

constexpr auto cat::is_aligned(intptr const& p_pointer, ssize const alignment)
    -> bool {
    return (p_pointer & (alignment - 1)) == 0;
}
