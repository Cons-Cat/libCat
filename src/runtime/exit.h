// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <numerals.h>

// TODO: Use isize here:
/* In libCat, the exit() function is provided globally instead of in <cstdlib>.
 * This streamlines out the existence of _exit(). */
extern "C" [[gnu::used]] void exit(int exit_code) {
    asm(R"(syscall)" : : "D"(meta::decay_numeral(exit_code)), "a"(60));
    __builtin_unreachable();  // This elides a ret instruction.
}
