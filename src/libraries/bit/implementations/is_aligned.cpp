// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <bit>

auto cat::is_aligned(intptr const& p_pointer, ssize const alignment) -> bool {
    return (alignment & static_cast<ssize>(p_pointer - 1)) == 0;
}
