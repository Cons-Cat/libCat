// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <simd>

// TODO: Document.
auto is_avx_supported() -> bool1 {
    return __builtin_cpu_supports("avx");
}
