// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::syscall3(int8 const call, cat::Any const arg1, cat::Any const arg2,
                   cat::Any const arg3) -> Result<cat::Any> {
    cat::Any result;
    asm volatile("syscall"
                 : "=a"(result)
                 : "a"(call), "D"(arg1), "S"(arg2), "d"(arg3)
                 // Clobbering all of these is necessary to prevent a segfault:
                 : "memory", "cc", "rcx", "r11");
    if (static_cast<int8>(result) < 0) {
        return Failure{meta::bit_cast<int8>(result)};
    }
    return result;
}
