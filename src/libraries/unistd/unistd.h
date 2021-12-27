// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <any.h>

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
auto syscall3(Any, Any, Any, Any) -> Result<Any>;
extern "C" auto syscall4(Any, Any, Any, Any, Any arg4) -> void*;
extern "C" auto syscall5(Any, Any, Any, Any, Any arg4, Any) -> void*;
auto syscall6(Any call, Any arg1, Any arg2, Any arg3, Any arg4, Any args,
              Any arg6) -> Result<Any>;

}  // namespace std::detail

template <typename ReturnType, typename T, typename... Args>
auto syscall(T call, Args... parameters) -> Result<ReturnType>
requires(sizeof...(Args) < 7);

// write() forwards its arguments to a failable stdout syscall.
auto write(i8 const& file_descriptor, char8_t const* p_string_buffer,
           isize const& string_length) -> Result<isize>;

#include "./impl/syscall.tpp"
