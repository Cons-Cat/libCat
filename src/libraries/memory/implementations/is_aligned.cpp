// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <memory>

auto std::is_aligned(void const volatile* pointer, ssize byte_alignment)
    -> bool {
    return (reinterpret_cast<usize>(pointer) % byte_alignment) == 0;
}
