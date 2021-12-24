// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <any.h>
#include <errno.h>

// TODO: Remove all trace of errno.

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
              Any arg6) -> Result<> {
    i4 result;
    asm volatile(R"(mov %[d],%%r10
                    mov %[e],%%r8
                    mov %[f],%%r9
                    syscall)"
                 : "=a"(result)
                 : "0"(any_cast<i4>(call)), "D"(arg1), "S"(arg2),
                   "d"(arg3), [d] "g"(arg4), [e] "g"(arg5), [f] "g"(arg6)
                 : "r11", "rcx", "r8", "r10", "r9", "memory");
    if (result < 0) {
        // The negative of `result` represents Linux's errno.
        return Failure(-result);
    }
    return okay;
}

}  // namespace std::detail

// TODO: Extract this to a .tpp file.
template <typename T, typename... Args>
auto syscall(T, Args... parameters) -> Result<> {
    constexpr i8 stdout = 1;
    constexpr isize length = sizeof...(Args);
    Any arguments[length] = {parameters...};
    static_assert(length < 7,
                  "Syscalls with more than 6 arguments are not supported!");

    using namespace std::detail;
    if constexpr (length == 0) {
        syscall0(stdout);
    } else if constexpr (length == 1) {
        syscall1(stdout, arguments[0]);
    } else if constexpr (length == 2) {
        syscall2(stdout, arguments[0], arguments[1]);
    } else if constexpr (length == 3) {
        syscall3(stdout, arguments[0], arguments[1], arguments[2]);
    } else if constexpr (length == 4) {
        syscall4(stdout, arguments[0], arguments[1], arguments[2],
                 arguments[3]);
    } else if constexpr (length == 5) {
        syscall5(stdout, arguments[0], arguments[1], arguments[2], arguments[3],
                 arguments[4]);
    } else if constexpr (length == 6) {
        return syscall6(stdout, arguments[0], arguments[1], arguments[2],
                        arguments[3], arguments[4], arguments[5]);
    }
    return okay;
}

// write() forwards its arguments to a failable stdout syscall.
auto write(i8 const& file_descriptor, char8_t const* p_string_buffer,
           isize const& string_length) -> Result<> {
    syscall(1, file_descriptor, p_string_buffer, string_length);
    return get_errno();
}
