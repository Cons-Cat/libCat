// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::syscall2(ssize const call, cat::Any const arg1, cat::Any const arg2)
    -> ssize {
    ssize result;
    asm volatile("syscall"
                 : "=a"(result)
                 : "a"(call), "D"(arg1), "S"(arg2)
                 // Clobbering all of these is necessary to prevent a segfault:
                 : "memory", "cc", "rcx", "r11");
    return result;
}
