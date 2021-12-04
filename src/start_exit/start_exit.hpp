#pragma once

#include <result.hpp>

#include <concepts>
#include <type_traits>

// template <typename T>
// struct Result;

// template struct Result<void>;

auto meow(i32 argc, char const* argv[], const void* argp) -> Result<void>;
auto meow(i32 argc, char const* argv[]) -> Result<void>;
auto meow(i32 argc) -> Result<void>;
auto meow() -> Result<void>;

void _exit(i32 exit_code) {
    // clang-format off
        asm (R"(syscall)"
           : // No outputs.
           : "D" (std::decay_integral(exit_code)), "a"(60)
           : "memory");
    // clang-format on
    __builtin_unreachable();  // This improves -O0.
}

/* extern "C" prevents C++ name mangling, which is required for the following
 * functions to link correcty. */

/* _start() is the entry point in a LibCat program.
 * I choose not to support .init, .fini, .ctor, or .dtor.
 * Thus, this is not a POSIX / ELF compliant alternative to
 * the LibC _start, but this is technically reasonable. */
extern "C" void _start() {
    // clang-format off
    asm(R"(xor %rbp, %rbp # Zero-out the stack pointer.
           mov %esp, %edi # Copy 32-bit argc
           lea 8(%rsp), %rsi # Load address to argv
/* LibCat user programs enter in meow() rather than main(). This is
 * done so that they may return a Result instead of an int. */
        )");
    // clang-format on

    // TODO: Output %esp and %edi into variables that are passed into meow().
    _exit(meow().code.code);
    __builtin_unreachable();  // This improves -O0.
}

/* __stack_chk_fail() is called when stack overflow occurs in programs compiled
 * without -fno-stack-protector. */
extern "C" void __stack_chk_fail() {
    _exit(1);
}
