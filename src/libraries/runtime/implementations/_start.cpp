#include <cat/runtime>

// Attributes on this prototype would have no effect.
auto main(...) -> int;

[[noreturn]] void call_main() {
	register int argc asm("rdi");
    register char** p_argv asm("rsi");
    main(argc, p_argv);
    __builtin_unreachable();
}

void _start() {
     // If `NO_ARGC_ARGV` is defined, argument loading can be skipped.
#ifndef NO_ARGC_ARGV
    asm(R"(pop %rdi        # Load `int4 argc`.
           mov %rsp, %rsi  # Load `char* argv[]`.
       )");
#endif

	// The stack pointer must be aligned to prevent SIMD segfaults even in
	// surprisingly simple programs with GCC 12.
    cat::align_stack_pointer_32();

    // `main()` must be wrapped by a function to conditionally prevent inlining.
    call_main();	
	__builtin_unreachable();
}
