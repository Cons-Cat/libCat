// -*- mode: c++ -*-
// vim: set ft=cpp:
// TODO: Move this into `<bit>`.
#include <memory>

auto cat::is_aligned(intptr const& p_pointer, ssize const alignment) -> bool1 {
    return (alignment & static_cast<ssize>(p_pointer - 1)) == 0;
}
