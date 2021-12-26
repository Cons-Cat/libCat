// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <any.h>
#include <errno.h>

// TODO: Remove all trace of `errno`.

/* For some reason, writing these syscalls with inline assembly produces bizarre
 * and broken codegen. */
namespace std::detail {

// TODO: Make all syscalls inline assembly. It did not work, last I tried.
/* The source to these functions is contained in the ./detail/ subdirectory at
 * the path of this file. They are split into individual files, because GCC
 * cannot exclude compiling unused functions individually from within one .s
 * file. */
// TODO: All syscalls should return a `Result`.
extern "C" auto syscall0(Any) -> void*;
extern "C" auto syscall1(Any, Any) -> void*;
extern "C" auto syscall2(Any, Any, Any) -> void*;
extern "C" auto syscall3(Any, Any, Any, Any) -> void*;
extern "C" auto syscall4(Any, Any, Any, Any, Any arg4) -> void*;
extern "C" auto syscall5(Any, Any, Any, Any, Any arg4, Any) -> void*;
auto syscall6(Any call, Any arg1, Any arg2, Any arg3, Any arg4, Any arg5,
              Any arg6) -> Result<Any> {
    register Any r10 asm("r10") = arg4;
    register Any r8 asm("r8") = arg5;
    register Any r9 asm("r9") = arg6;

    Any result;
    asm("syscall"
        : "=a"(result)
        : "a"(call), "D"(arg1), "S"(arg2), "d"(arg3), "r"(r10), "r"(r8),
          "r"(r9),
          // TODO: Fix this segfaulting with static_cast<void*>():
          [a6] "re"(meta::bit_cast<void*>(arg6))
        : "rcx", "r11", "memory", "cc");
    if (meta::bit_cast<i8>(result) < 0) {
        // The negative of `result` represents Linux's errno.
        return Failure(-meta::bit_cast<i8>(result));
    }
    return result;
}

}  // namespace std::detail

// TODO: Extract this to a .tpp file.
template <typename ReturnType, typename T, typename... Args>
auto syscall(T call, Args... parameters) -> Result<ReturnType>
requires(sizeof...(Args) < 7) {
    static constexpr isize length = sizeof...(Args);
    Any arguments[length] = {parameters...};

    using namespace std::detail;
    if constexpr (length == 0) {
        syscall0(call);
        return okay;
    } else if constexpr (length == 1) {
        syscall1(call, arguments[0]);
        return okay;
    } else if constexpr (length == 2) {
        syscall2(call, arguments[0], arguments[1]);
        return okay;
    } else if constexpr (length == 3) {
        syscall3(call, arguments[0], arguments[1], arguments[2]);
        return okay;
    } else if constexpr (length == 4) {
        syscall4(call, arguments[0], arguments[1], arguments[2], arguments[3]);
        return okay;
    } else if constexpr (length == 5) {
        syscall5(call, arguments[0], arguments[1], arguments[2], arguments[3],
                 arguments[4]);
        return okay;
    } else if constexpr (length == 6) {
        return syscall6(call, arguments[0], arguments[1], arguments[2],
                        arguments[3], arguments[4], arguments[5]);
    }
    __builtin_unreachable();
}

// write() forwards its arguments to a failable stdout syscall.
auto write(i8 const& file_descriptor, char8_t const* p_string_buffer,
           isize const& string_length) -> Result<> {
    // return syscall<void>(1, file_descriptor, p_string_buffer, string_length);
    return syscall<void>(1, file_descriptor, p_string_buffer, string_length, 0,
                         0, 0);
}
