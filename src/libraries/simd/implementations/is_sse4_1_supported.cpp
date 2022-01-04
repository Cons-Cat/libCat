// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <simd>

// TODO: Document.
auto is_sse4_1_supported() -> bool {
    return __builtin_cpu_supports("sse4.1");
}
