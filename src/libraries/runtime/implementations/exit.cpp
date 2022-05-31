#include <cat/runtime>

// Terminate the program. Without arguments, this exits with a success code for
// the target operating system.
[[gnu::used, noreturn]] void cat::exit(ssize exit_code) {
    asm("syscall" : : "D"(exit_code), "a"(60));
    __builtin_unreachable();  // This elides a `ret` instruction.
}
