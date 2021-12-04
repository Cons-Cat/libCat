#pragma once

#include <result.hpp>

#include <concepts>
#include <cstdlib>
#include <type_traits>

/* meow() is the entry point in a libCat program, rather than main(). Why?
 *
 * This allows libCat to try some interesting ideas, such as enabling the user
 * to express explicit stack initialization and giving them generally much more
 * control over their program than a libC can.
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
 * assumption that libC is simpley how computers work. It's not. libC is simply
 * a collection of idioms. Here are alternatives I think could be good: */
void meow();

/* In libCat, the exit() function is provided globally instead of in <cstdlib>.
 * This streamlines out the existence of _exit(). */
void exit(i32 exit_code = EXIT_SUCCESS) {
    // clang-format off
    asm (R"(syscall)"
           : // No outputs.
           : "D" (std::decay_integral(exit_code)), "a"(60)
           : "memory");
    // clang-format on
    // __builtin_unreachable();
}

/* In libCat, _start() performs no initialization code. It simply invokes
 * meow(), and subsequently exits. This only exists to streamline the build
 * systems and to automate exiting. */
extern "C" void _start() {
    meow();
    exit();
    //     // clang-format off
    //     asm(R"(xor %rbp, %rbp # Zero-out the stack pointer.
    //            mov %esp, %edi # Copy 32-bit argc
    //            lea 8(%rsp), %rsi # Load address to argv
    // /* libCat user programs enter in meow() rather than main(). This is
    //  * done so that they may return a Result instead of an int. */
    //         )");
    //     // clang-format on
}

/* __stack_chk_fail() is called when stack overflow occurs in programs compiled
 * without -fno-stack-protector. */
extern "C" void __stack_chk_fail() {
    exit(1);
}
