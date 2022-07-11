#include <cat/linux>

auto nix::syscall5(ssize call, cat::Any arg1, cat::Any arg2, cat::Any arg3,
                   cat::Any const arg4, cat::Any const arg5) -> ssize {
    // `arg4` and `arg5` must be `const-qualified` for GCC to compile.
    register cat::Any const r10 asm("r10") = arg4;
    register cat::Any const r8 asm("r8") = arg5;

    ssize result;
    asm volatile("syscall"
                 : "=a"(result)
                 : "a"(call), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10), "r"(r8)
                 // Clobbering all of these is necessary to prevent a segfault:
                 : "rcx", "r11", "memory", "cc");
    return result;
}
