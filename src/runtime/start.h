// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <concepts.h>
#include <type_traits.h>

/* meow() is the user's entry point in a libCat program, rather than main().
 * This allows libCat to explore interesting ideas, such as enabling the user
 * to express explicit stack initialization, and giving them generally more
 * control over their program than the standard `crt0` or `crt1` do.
 *
 * That control is very generally useful. Do you need to support `.init` and
 * `.fini`, or do you only use `.ctor` or `.dtor` instead? Or both? Or neither?
 * POSIX and ELF require libC support all four, technically, but there is
 * usually no tangible reason to do so.
 *
 * Does your program need to allocate a stack at all, or can you just use the
 * space you're already given in `argv[]`? Do you have to populate `argp[]`?
 * Does your program need to create a main pthread, or not? Should you align the
 * stack, or is that unnecessary? There are myriad creative liberties available
 * once you rethink basic assumptions about what C++ actually does.
 * libC is merely a suite of idioms, not our language semantics, so libCat
 * explores one alternative.
 *
 * As far as the name change from main() goes, GCC simply won't compile when it
 * thinks the main() definition is ill-formed, so this change *is* necessary
 * for fundamentally different behavior, such as not returning `int`. Meow is
 * four letters, it starts with 'm', and it sounds cute, so it's as good as
 * anything else. */

/* TODO: #ifdef __SANITIZE_ADDRESS__
 * to handle address sanitizers, which do not work. */

/* in libCat, `_start()` does almost nothing. It is necessary to store the
 * base stack pointer for future use, because it is impossible after memory is
 * pushed to the stack, but otherwise all initialization logic shall occur in
 * meow(). */
extern "C" __attribute__((used)) void _start() {
    /* This cannot be simplified any further without producing unreliable
     * codegen. */
    asm(R"(mov %rsp, %rbp
           call meow
           mov $0, %edi
           call exit)");
    __builtin_unreachable();  // This elides a ret instruction.
}

auto load_base_stack_pointer() -> void* {
    void* rbp;
    asm("mov %%rbp, %[rbp]" : [rbp] "=r"(rbp));
    return rbp;
}

auto load_argc() -> i4 {
    i4* argc;
    asm("mov %%rbp, %[argc]" : [argc] "=r"(argc));
    return *argc;
}

auto load_argv() -> char8_t const** {
    char8_t const** argv;
    asm("mov 4(%%rbp), %0" : "=r"(argv));
    return argv;
}

void align_stack_pointer_16() {
    asm("and $-16, %rsp");
}

void align_stack_pointer_32() {
    asm("and $-32, %rsp");
}

/* __stack_chk_fail() is called when stack overflow occurs in programs compiled
 * without -fno-stack-protector. */
extern "C" void __stack_chk_fail() {
    exit(1);
}
