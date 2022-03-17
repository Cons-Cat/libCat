// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <runtime>

// Terminate the program. Without arguments, this exits with a success code for
// the target operating system.
extern "C" [[gnu::used]] void std::exit(ssize exit_code) {
    // TODO: is `meta::decay_numeral()` needed here? Why?
    asm("syscall" : : "D"(meta::decay_numeral(exit_code)), "a"(60));
    __builtin_unreachable();  // This elides a `ret` instruction.
}
