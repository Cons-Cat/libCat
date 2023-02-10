#include <cat/linux>

auto nix::syscall4(cat::iword call, cat::no_type arg1, cat::no_type arg2,
                   cat::no_type arg3, cat::no_type const arg4) -> cat::iword {
    // `arg4` must be `const-qualified` for GCC to compile.
    register cat::no_type const r10 asm("r10") = arg4;

    cat::iword result;
    asm volatile("syscall"
                 : "=a"(result)
                 : "a"(call), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10)
                 // Clobbering all of these is necessary to prevent a segfault:
                 : "rcx", "r11", "memory", "cc");
    return result;
}
