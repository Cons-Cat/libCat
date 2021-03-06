#include <cat/linux>

auto nix::syscall6(ssize call, cat::Any arg1, cat::Any arg2, cat::Any arg3,
                   cat::Any const arg4, cat::Any const arg5,
                   cat::Any const arg6) -> ssize {
    // `arg4`, `arg5`, and `arg6` must be `const-qualified` for GCC to compile.
    register cat::Any r10 asm("r10") = arg4;
    register cat::Any r8 asm("r8") = arg5;
    register cat::Any r9 asm("r9") = arg6;

    ssize result;
    asm volatile("syscall"
                 : "=a"(result)
                 : "a"(call), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10),
                   "r"(r8), "r"(r9),
                   // TODO: Fix this segfaulting with static_cast<void*>():
                   [a6] "re"(cat::bit_cast<void*>(arg6))
                 : "rcx", "r11", "memory", "cc");
    return result;
}
