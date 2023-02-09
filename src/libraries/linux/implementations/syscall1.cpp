#include <cat/linux>

auto nix::syscall1(iword call, cat::no_type arg) -> iword {
    iword result;
    asm volatile("syscall"
                 : "=a"(result)
                 : "a"(call), "D"(arg)
                 // Clobbering all of these is necessary to prevent a segfault:
                 : "memory", "cc", "rcx", "r11");
    return result;
}
