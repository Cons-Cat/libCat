// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::syscall6(int8 const call, cat::Any const arg1, cat::Any const arg2,
                   cat::Any const arg3, cat::Any const arg4,
                   cat::Any const arg5, cat::Any const arg6)
    -> Result<cat::Any> {
    register cat::Any r10 asm("r10") = arg4;
    register cat::Any r8 asm("r8") = arg5;
    register cat::Any r9 asm("r9") = arg6;

    cat::Any result;
    asm volatile("syscall"
                 : "=a"(result)
                 : "a"(call), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10),
                   "r"(r8), "r"(r9),
                   // TODO: Fix this segfaulting with static_cast<void*>():
                   [a6] "re"(meta::bit_cast<void*>(arg6))
                 : "rcx", "r11", "memory", "cc");
    if (static_cast<int8>(result) < 0) {
        return Failure{meta::bit_cast<int8>(result)};
    }
    return result;
}
