// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <simd>

// TODO: Document.
auto is_avx512f_supported() -> bool {
    __builtin_cpu_init();
    return __builtin_cpu_supports("avx512f");
}
