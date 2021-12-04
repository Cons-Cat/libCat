#pragma once

#include <result.hpp>

#include <concepts>
#include <cstdlib>
#include <type_traits>

/* meow() is the entry point in a libCat program, rather than main(). Why?
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
 * all? Does your program need to create a main pthread, or not? There are
 * myriad creative liberties suddenly available once you rethink the basic
 * assumption that libC is simply the computer works. It's not, of course. libC
 * is only a collection of idioms, not our language semantics. Let's explore
 * some alternatives.
 *
 * As far as the name change from main() goes, GCC simply won't compile if it
 * thinks your main() definition is incorrect, so a name change *is* necessary
 * for fundamentally different behavior, such as not returning int. */

/* In libCat, the exit() function is provided globally instead of in <cstdlib>.
 * This streamlines out the existence of _exit(). */
extern "C" void exit(i32 exit_code) {
    asm(R"(syscall)" : : "D"(std::decay_integral(exit_code)), "a"(60));
    __builtin_unreachable();  // This elides a retq instruction.
}

/* in libCat, _start() does almost nothing. It is necessary to store the base
 * stack pointer at this point, but otherwise all initialization logic occurs in
 * meow(). */
extern "C" __attribute__((used)) void _start() {
    asm volatile(R"(mov %rsp, %rbp
                    call meow
                    mov $0, %edi
                    call exit)");
    __builtin_unreachable();  // This elides a retq instruction.
}

auto load_argc() -> i32 {
    i32* argc;
    asm(R"(mov %%rbp, %[argc])" : [argc] "=r"(argc));
    return *argc;
}

/* __stack_chk_fail() is called when stack overflow occurs in programs compiled
 * without -fno-stack-protector. */
extern "C" void __stack_chk_fail() {
    exit(1);
}
