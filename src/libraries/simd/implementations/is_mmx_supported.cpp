// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <simd>

// TODO: Document.
auto is_mmx_supported() -> bool {
    return __builtin_cpu_supports("mmx");
}
