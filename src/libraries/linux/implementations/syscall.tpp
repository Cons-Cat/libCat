// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/linux>

template <typename T, typename... Args>
auto nix::syscall(cat::iword call, Args... parameters) -> nix::scaredy_nix<T>
    requires(sizeof...(Args) < 7)
{
    static constexpr unsigned long length = sizeof...(Args);
    cat::no_type arguments[length] = {parameters...};

    // TODO: Make this a `union` of reasonable types.
    cat::no_type result;

    if constexpr (length == 0) {
        result = nix::syscall0(call);
    } else if constexpr (length == 1) {
        result = nix::syscall1(call, arguments[0]);
    } else if constexpr (length == 2) {
        result = nix::syscall2(call, arguments[0], arguments[1]);
    } else if constexpr (length == 3) {
        result = nix::syscall3(call, arguments[0], arguments[1], arguments[2]);
    } else if constexpr (length == 4) {
        result = nix::syscall4(call, arguments[0], arguments[1], arguments[2],
                               arguments[3]);
    } else if constexpr (length == 5) {
        result = nix::syscall5(call, arguments[0], arguments[1], arguments[2],
                               arguments[3], arguments[4]);
    } else {
        result = nix::syscall6(call, arguments[0], arguments[1], arguments[2],
                               arguments[3], arguments[4], arguments[5]);
    }

    if (static_cast<cat::iword>(result) < 0) {
        return static_cast<linux_error>(result);
    }
    if constexpr (cat::is_void<T>) {
        return monostate;
    } else {
        return static_cast<T>(result);
    }
}
