// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::syscall0(int8 const call) -> Result<cat::Any> {
    cat::Any result;
    asm volatile("syscall"
                 : "=a"(result)
                 : "a"(call)
                 // Clobbering all of these is necessary to prevent a segfault:
                 : "memory", "cc", "rcx", "r11");
    if (static_cast<ssize>(result) < 0) {
        return Failure{meta::bit_cast<int8>(result)};
    }
    return result;
}
