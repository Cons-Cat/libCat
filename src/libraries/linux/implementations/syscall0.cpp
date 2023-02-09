#include <cat/linux>

auto nix::syscall0(iword call) -> iword {
    iword result;
    asm volatile("syscall"
                 : "=a"(result)
                 : "a"(call)
                 // Clobbering all of these is necessary to prevent a segfault:
                 : "memory", "cc", "rcx", "r11");
    return result;
}
