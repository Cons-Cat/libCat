// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <simd.h>

// TODO: Document.
auto is_avx2_supported() -> bool {
    return __builtin_cpu_supports("avx2");
}
