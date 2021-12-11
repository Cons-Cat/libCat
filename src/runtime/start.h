#pragma once

#include <concepts.h>
#include <numerals.h>
#include <type_traits.h>

/* meow() is the user's entry point in a libCat program, rather than main().
 *
 * This allows libCat to explore interesting ideas, such as enabling the user
 * to express explicit stack initialization and giving them generally much more
 * control over their program than crt0 / crt1 do.
 *
 * That control is very generally useful. Do you need to support .init and
 * .fini, or do you only use .ctor or .dtor instead? Or both? Or neither? POSIX
 * and ELF require libC support all four, technically, but there is no good
 * reason to actually do that. Very few programs actually need any of them.
 *
 * Does your program need to allocate a stack at all, or can you just use the
 * space you're already given in argv[]? Do you even have to populate argp[] at
 * all? Does your program need to create a main pthread, or not? Should you
 * align the stack, or is that unnecessary? There are myriad creative liberties
 * available once you rethink the basic assumption that libC is simply the
 * computer works. libC is merely a suite of idioms, not our language semantics.
 * libCat explores alternatives.
 *
 * As far as the name change from main() goes, GCC simply won't compile if it
 * thinks the main() definition is ill-formed, so a name change *is* necessary
 * for fundamentally different behavior, such as not returning int. Meow is four
 * letters, it starts with 'm', and it sounds cute, so it's as good as anything
 * else. */

/* TODO: #ifdef __SANITIZE_ADDRESS__
 * to handle address sanitizers, which do not work. */

extern "C" __attribute__((used)) void exit(i32);

/* in libCat, _start() does almost nothing. It is necessary to store the
 * base stack pointer at this point, but otherwise all initialization logic
 * occurs in meow(). */
extern "C" __attribute__((used)) void _start() {
    asm volatile(R"(mov %rsp, %rbp
                    call meow
                    mov $0, %edi
                    call exit)");
    __builtin_unreachable();  // This elides a ret instruction.
}

auto load_base_stack_pointer() -> void* {
    void* rbp;
    asm(R"(mov %%rbp, %[rbp])" : [rbp] "=r"(rbp));
    return rbp;
}

auto load_argc() -> i32 {
    i32* argc;
    asm(R"(mov %%rbp, %[argc])" : [argc] "=r"(argc));
    return *argc;
}

auto load_argv() -> char const** {
    char const** argv;
    asm("mov 4(%%rbp), %0" : "=r"(argv));
    return argv;
}

/* __stack_chk_fail() is called when stack overflow occurs in programs compiled
 * without -fno-stack-protector. */
extern "C" void __stack_chk_fail() {
    exit(1);
}
