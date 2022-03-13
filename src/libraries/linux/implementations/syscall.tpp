// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <linux>

template <typename ReturnType, typename... Args>
auto nix::syscall(Any call, Args... parameters) -> Result<ReturnType>
requires(sizeof...(Args) < 7) {
    static constexpr isize length = sizeof...(Args);
    Any arguments[length] = {parameters...};

    if constexpr (length == 0) {
        return nix::syscall0(call);
    } else if constexpr (length == 1) {
        return nix::syscall1(call, arguments[0]);
    } else if constexpr (length == 2) {
        return nix::syscall2(call, arguments[0], arguments[1]);
    } else if constexpr (length == 3) {
        return nix::syscall3(call, arguments[0], arguments[1], arguments[2]);
    } else if constexpr (length == 4) {
        return nix::syscall4(call, arguments[0], arguments[1], arguments[2],
                             arguments[3]);
    } else if constexpr (length == 5) {
        return nix::syscall5(call, arguments[0], arguments[1], arguments[2],
                             arguments[3], arguments[4]);
    } else if constexpr (length == 6) {
        return nix::syscall6(call, arguments[0], arguments[1], arguments[2],
                             arguments[3], arguments[4], arguments[5]);
    }
    __builtin_unreachable();
}
