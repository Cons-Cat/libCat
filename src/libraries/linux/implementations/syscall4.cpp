#include <cat/linux>

auto nix::syscall4(ssize call, cat::Any arg1, cat::Any arg2, cat::Any arg3,
                   cat::Any const arg4) -> ssize {
    // `arg4` must be `const-qualified` for GCC to compile.
    register cat::Any const r10 asm("r10") = arg4;

    ssize result;
    asm volatile("syscall"
                 : "=a"(result)
                 : "a"(call), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10)
                 // Clobbering all of these is necessary to prevent a segfault:
                 : "rcx", "r11", "memory", "cc");
    return result;
}
