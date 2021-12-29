// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <runtime.h>

// Linux requires an unsigned 8-byte integer exit code for this syscall.
extern "C" [[gnu::used]] void exit(i8 exit_code) {
    // TODO: is `meta::decay_numeral()` needed here? Why?
    asm("syscall" : : "D"(meta::decay_numeral(exit_code)), "a"(60));
    __builtin_unreachable();  // This elides a ret instruction.
}
