// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <simd.h>

// TODO: Document.
auto is_sse3_supported() -> bool {
    return __builtin_cpu_supports("sse3");
}