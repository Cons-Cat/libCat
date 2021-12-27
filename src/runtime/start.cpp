extern "C" [[gnu::used]] void _start() {
    /* This cannot be simplified any further without producing unreliable
     * codegen. */
    asm(R"(mov %rsp, %rbp
           call meow)");
    __builtin_unreachable();  // This elides a `ret` instruction.
}

auto load_base_stack_pointer() -> void* {
    void* rbp;
    asm("mov %%rbp, %[rbp]" : [rbp] "=r"(rbp));
    return rbp;
}

auto load_argc() -> i4 {
    i4 argc;
    asm("mov 0(%%rbp), %[argc]" : [argc] "=r"(argc));
    return argc;
}

auto load_argv() -> char8_t** {
    char8_t** argv;
    asm("lea 8(%%rbp), %[argv]" : [argv] "=r"(argv));
    return argv;
}

void align_stack_pointer_16() {
    asm("and $-16, %rsp");
}

void align_stack_pointer_32() {
    asm("and $-32, %rsp");
}

void dont_optimize_out(auto& variable) {
    asm volatile("" ::"m"(variable));
}

/* `__stack_chk_fail()` is called when stack overflow occurs in programs
 * compiled without `-fno-stack-protector`. */
extern "C" void __stack_chk_fail() {
    exit(1);
}
