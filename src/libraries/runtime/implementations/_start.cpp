#include <cat/runtime>

extern "C" [[gnu::used]] void _start() {
#ifdef LOAD_ARGC_ARGV
    asm(R"(pop %rdi        # Load `int4 argc`.
           mov %rsp, %rsi  # Load `char* argv[]`.
       )");
#endif
    // The stack pointer must be aligned to prevent segfaults even in "Hello
    // World".
    cat::align_stack_pointer_32();
    meow();
    __builtin_unreachable();  // This elides a `ret` instruction.
}
