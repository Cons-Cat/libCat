// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <memory>

auto std::is_aligned(void const volatile* p_pointer, ssize byte_alignment)
    -> bool1 {
    return (reinterpret_cast<usize>(p_pointer) % byte_alignment) == 0;
}
