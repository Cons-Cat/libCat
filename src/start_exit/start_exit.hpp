#pragma once

#include <result.hpp>

#include <concepts>
#include <cstdlib>
#include <type_traits>

/* meow() is the entry point in a libCat program, so there is no _start().
 *
 * This allows libCat to do some interesting things, such as making stack
 * initialization explicit and giving the user more control over their program
 * than a libC.
 *
 * That is very generally useful. Do you need to support .init and .fini, or do
 * you use .ctor or .dtor? Or both? Or neither? POSIX and ELF require libC
 * support all four, technically, but there is no good reason to actually do
 * that.
 *
 * Does your program need to allocate a stack at all, or can you just use
 * argv[]? Do you even have to populate argp[] at all? There are many creative
 * liberties that open up when you rethink the basic assumption that libC is
 * just how computers work. It's not. They're just idioms. */
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
    __builtin_unreachable();
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
    // /* LibCat user programs enter in meow() rather than main(). This is
    //  * done so that they may return a Result instead of an int. */
    //         )");
    //     // clang-format on
}

/* __stack_chk_fail() is called when stack overflow occurs in programs compiled
 * without -fno-stack-protector. */
extern "C" void __stack_chk_fail() {
    exit(1);
}
