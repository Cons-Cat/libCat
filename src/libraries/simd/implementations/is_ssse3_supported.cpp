// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <simd>

// TODO: Document.
auto is_ssse3_supported() -> bool {
    return __builtin_cpu_supports("ssse3");
}
