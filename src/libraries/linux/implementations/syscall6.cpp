#include <cat/linux>

auto nix::syscall6(ssize call, cat::NoType arg1, cat::NoType arg2,
                   cat::NoType arg3, cat::NoType const arg4,
                   cat::NoType const arg5, cat::NoType const arg6) -> ssize {
    // `arg4`, `arg5`, and `arg6` must be `const-qualified` for GCC to compile.
    register cat::NoType r10 asm("r10") = arg4;
    register cat::NoType r8 asm("r8") = arg5;
    register cat::NoType r9 asm("r9") = arg6;

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
