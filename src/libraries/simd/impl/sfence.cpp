// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <simd.h>

void simd::sfence() {
    __builtin_ia32_sfence();
}
