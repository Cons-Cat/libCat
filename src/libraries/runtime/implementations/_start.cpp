#include <cat/runtime>

extern "C" [[gnu::used]] void _start() {
#ifdef load_argc_argv
    asm(R"(pop %rdi        # Load `int4 argc`.
           mov %rsp, %rsi  # Load `char* argv[]`.
           and $-16, %rsp
           call meow)");
#else
    // The stack pointer must be aligned to prevent segfaults even in "Hello
    // World".
    cat::align_stack_pointer_32();
    meow();
#endif
    __builtin_unreachable();  // This elides a `ret` instruction.
}
