#include <cat/runtime>

extern "C" [[gnu::used, gnu::always_inline]] inline void _start() {
#ifdef LOAD_ARGC_ARGV
    asm(R"(pop %rdi        # Load `int4 argc`.
           mov %rsp, %rsi  # Load `char* argv[]`.
       )");
#endif
	
    // The stack pointer must be aligned to prevent segfaults even in "Hello
    // World".
    cat::align_stack_pointer_32();
	
    // Calling `meow()` via `asm` allows it to have optional stack arguments.
    asm("call meow");

	// `__builtin_unreachable()` elides a `ret` instruction here.
#ifndef LOAD_ARGC_ARGV
	// This causes argc and argv to fail loading when UBsan is used, because
	// stack space is reserved before they can be loaded. Genius GNU developers
	// refuse to support a `__SANITIZE_UNDEFINED__` macro to fix this.
    __builtin_unreachable();
#elif __OPTIMIZE__
	__builtin_unreachable();
#endif
}
