// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::syscall5(int8 const call, cat::Any const arg1, cat::Any const arg2,
                   cat::Any const arg3, cat::Any const arg4,
                   cat::Any const arg5) -> Result<cat::Any> {
    register cat::Any r10 asm("r10") = arg4;
    register cat::Any r8 asm("r8") = arg5;

    cat::Any result;
    asm volatile("syscall"
                 : "=a"(result)
                 : "a"(call), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10), "r"(r8)
                 // Clobbering all of these is necessary to prevent a segfault:
                 : "rcx", "r11", "memory", "cc");
    if (static_cast<int8>(result) < 0) {
        return Failure{meta::bit_cast<int8>(result)};
    }
    return result;
}
