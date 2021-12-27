// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <simd.h>

// TODO: Document.
void simd::zero_upper_avx_registers() {
    __builtin_ia32_vzeroupper();
}
