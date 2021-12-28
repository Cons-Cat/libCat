// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <simd.h>

// TODO: Document.
auto is_ssse3_supported() -> bool {
    return __builtin_cpu_supports("ssse3");
}
