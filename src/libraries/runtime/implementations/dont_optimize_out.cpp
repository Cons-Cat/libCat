// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <runtime>

/* This function wraps a volatile no-op instruction that will prevent many
 * optimizations such as loop unrolling, or constant-folding out a variable
 * passed into this. */
void dont_optimize_out(auto& variable) {
    asm volatile("nop" ::"m"(variable));
}
