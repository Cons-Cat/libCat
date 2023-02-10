#include <cat/linux>

auto nix::syscall0(cat::iword call) -> cat::iword {
    cat::iword result;
    asm volatile("syscall"
                 : "=a"(result)
                 : "a"(call)
                 // Clobbering all of these is necessary to prevent a segfault:
                 : "memory", "cc", "rcx", "r11");
    return result;
}
