// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <runtime>

extern "C" [[gnu::used]] void _start() {
    /* This cannot be simplified any further without producing pessimized
     * codegen. */
    asm(R"(mov %rsp, %rbp
           call meow)");
    __builtin_unreachable();  // This elides a `ret` instruction.
}
