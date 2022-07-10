#include <cat/runtime>

// Attributes on this prototype would have no effect.
[[noreturn]] int main(...);

[[noreturn
  // `main()` cannot be inlined if arguments are loaded.
#ifdef LOAD_ARGC_ARGV
  , gnu::noinline
#endif
  ]] void call_main() {
    register int argc asm("rdi");
    register char** p_argv asm("rsi");
    main(argc, p_argv);
    __builtin_unreachable();
}

// A frame pointer here prevents arguments from being loaded.
#pragma GCC push_options
#pragma GCC optimize("omit-frame-pointer")

void _start() {
#ifdef LOAD_ARGC_ARGV
#ifdef __SANITIZE_ADDRESS__
	// The sanitizers are inserting a `sub` instruction here, which breaks
	// argument loading. Compensate by inserting an `add`.
    asm("add $8,%rsp");
#endif
    asm(R"(pop %rdi        # Load `int4 argc`.
           mov %rsp, %rsi  # Load `char* argv[]`.
       )");
#endif

    // The stack pointer must be aligned to prevent segfaults even in "Hello
    // World".
    cat::align_stack_pointer_32();

	// `main()` must be wrapped by a function to conditionally prevent inlining.
	call_main();
}

#pragma GCC pop_options
