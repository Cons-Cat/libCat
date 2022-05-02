// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::syscall0(ssize const call) -> ssize {
    ssize result;
    asm volatile("syscall"
                 : "=a"(result)
                 : "a"(call)
                 // Clobbering all of these is necessary to prevent a segfault:
                 : "memory", "cc", "rcx", "r11");
    return result;
}
