// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <bit>

constexpr auto align_up(intptr const& value, ssize const alignment) -> intptr {
    return (value + (alignment - 1)) & (~(alignment - 1));
}
