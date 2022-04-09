// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

// These syscalls must be `volatile`.

auto nix::syscall0(int8 const call) -> Result<Any> {
    Any result;
    asm volatile(
        "syscall"
        : "=a"(result)
        : "a"(call)
        /* Clobbering all of these is necessary to prevent a segfault: */
        : "memory", "cc", "rcx", "r11");
    if (meta::bit_cast<int8>(result) < 0) {
        // The negative of `result` represents Linux's errno.
        return Failure(-meta::bit_cast<int8>(result));
    }
    return result;
}

auto nix::syscall1(int8 const call, Any const arg) -> Result<Any> {
    Any result;
    asm volatile(
        "syscall"
        : "=a"(result)
        : "a"(call), "D"(arg)
        /* Clobbering all of these is necessary to prevent a segfault: */
        : "memory", "cc", "rcx", "r11");
    if (meta::bit_cast<int8>(result) < 0) {
        // The negative of `result` represents Linux's errno.
        return Failure(-meta::bit_cast<int8>(result));
    }
    return result;
}

auto nix::syscall2(int8 const call, Any const arg1, Any const arg2)
    -> Result<Any> {
    Any result;
    asm volatile(
        "syscall"
        : "=a"(result)
        : "a"(call), "D"(arg1), "S"(arg2)
        /* Clobbering all of these is necessary to prevent a segfault: */
        : "memory", "cc", "rcx", "r11");
    if (meta::bit_cast<int8>(result) < 0) {
        // The negative of `result` represents Linux's errno.
        return Failure(-meta::bit_cast<int8>(result));
    }
    return result;
}

auto nix::syscall3(int8 const call, Any const arg1, Any const arg2,
                   Any const arg3) -> Result<Any> {
    Any result;
    // `volatile` is required with optimizations on.
    asm volatile(
        "syscall"
        : "=a"(result)
        : "a"(call), "D"(arg1), "S"(arg2), "d"(arg3)
        /* Clobbering all of these is necessary to prevent a segfault: */
        : "memory", "cc", "rcx", "r11");
    if (meta::bit_cast<int8>(result) < 0) {
        // The negative of `result` represents Linux's errno.
        return Failure(-meta::bit_cast<int8>(result));
    }
    return result;
}

auto nix::syscall4(int8 const call, Any const arg1, Any const arg2,
                   Any const arg3, Any const arg4) -> Result<Any> {
    register Any r10 asm("r10") = arg4;

    Any result;
    asm volatile("syscall"
                 : "=a"(result)
                 : "a"(call), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10)
                 : "rcx", "r11", "memory", "cc");
    if (meta::bit_cast<int8>(result) < 0) {
        // The negative of `result` represents Linux's errno.
        return Failure(-meta::bit_cast<int8>(result));
    }
    return result;
}

auto nix::syscall5(int8 const call, Any const arg1, Any const arg2,
                   Any const arg3, Any const arg4, Any const arg5)
    -> Result<Any> {
    register Any r10 asm("r10") = arg4;
    register Any r8 asm("r8") = arg5;

    Any result;
    asm volatile("syscall"
                 : "=a"(result)
                 : "a"(call), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10), "r"(r8)
                 : "rcx", "r11", "memory", "cc");
    if (meta::bit_cast<int8>(result) < 0) {
        // The negative of `result` represents Linux's errno.
        return Failure(-meta::bit_cast<int8>(result));
    }
    return result;
}

auto nix::syscall6(int8 const call, Any const arg1, Any const arg2,
                   Any const arg3, Any const arg4, Any const arg5,
                   Any const arg6) -> Result<Any> {
    register Any r10 asm("r10") = arg4;
    register Any r8 asm("r8") = arg5;
    register Any r9 asm("r9") = arg6;

    Any result;
    asm volatile("syscall"
                 : "=a"(result)
                 : "a"(call), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10),
                   "r"(r8), "r"(r9),
                   // TODO: Fix this segfaulting with static_cast<void*>():
                   [a6] "re"(meta::bit_cast<void*>(arg6))
                 : "rcx", "r11", "memory", "cc");
    if (meta::bit_cast<int8>(result) < 0) {
        // The negative of `result` represents Linux's errno.
        return Failure(-meta::bit_cast<int8>(result));
    }
    return result;
}
