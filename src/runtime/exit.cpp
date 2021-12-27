#include <exit.h>

extern "C" [[gnu::used]] void exit(int exit_code) {
    asm(R"(syscall)" : : "D"(meta::decay_numeral(exit_code)), "a"(60));
    __builtin_unreachable();  // This elides a ret instruction.
}
