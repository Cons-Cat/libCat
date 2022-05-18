#include <cat/linux>

auto nix::syscall3(ssize const call, cat::Any const arg1, cat::Any const arg2,
                   cat::Any const arg3) -> ssize {
    ssize result;
    asm volatile("syscall"
                 : "=a"(result)
                 : "a"(call), "D"(arg1), "S"(arg2), "d"(arg3)
                 // Clobbering all of these is necessary to prevent a segfault:
                 : "memory", "cc", "rcx", "r11");
    return result;
}
