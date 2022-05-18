#include <cat/linux>

auto nix::syscall4(ssize const call, cat::Any const arg1, cat::Any const arg2,
                   cat::Any const arg3, cat::Any const arg4) -> ssize {
    register cat::Any r10 asm("r10") = arg4;

    ssize result;
    asm volatile("syscall"
                 : "=a"(result)
                 : "a"(call), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10)
                 // Clobbering all of these is necessary to prevent a segfault:
                 : "rcx", "r11", "memory", "cc");
    return result;
}
