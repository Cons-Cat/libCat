// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <bit>

constexpr auto align_down(intptr const& value, ssize const alignment)
    -> intptr {
    return value & (~(alignment - 1));
}
