#pragma once

#include <any.h>
#include <concepts.h>
#include <errno.h>

/* For some reason, writing these with inline assembly produces incredibly
 * strange codegen. */
namespace std::detail {

/* The source to these functions is contained in the ./detail/ subdirectory at
 * the path of this file. They are split into individual files, because GCC
 * cannot exclude compiling unused functions individually from within one .s
 * file. */
extern "C" auto syscall0(Any) -> void*;
extern "C" auto syscall1(Any, Any) -> void*;
extern "C" auto syscall2(Any, Any, Any) -> void*;
extern "C" auto syscall3(Any, Any, Any, Any) -> void*;
extern "C" auto syscall4(Any, Any, Any, Any, Any arg4) -> void*;
extern "C" auto syscall5(Any, Any, Any, Any, Any arg4, Any) -> void*;

}  // namespace std::detail

template <typename T, typename... Args>
void syscall(T, Args... parameters) {
    constexpr i64 stdout = 1;
    constexpr usize length = sizeof...(Args);
    Any arguments[length] = {parameters...};
    static_assert(length < 6);

    using namespace std::detail;
    if constexpr (length == 0) {
        syscall0(stdout);
    }
    if constexpr (length == 1) {
        syscall1(stdout, arguments[0]);
    }
    if constexpr (length == 2) {
        syscall2(stdout, arguments[0], arguments[1]);
    }
    if constexpr (length == 3) {
        syscall3(stdout, arguments[0], arguments[1], arguments[2]);
    }
    if constexpr (length == 4) {
        syscall4(stdout, arguments[0], arguments[1], arguments[2],
                 arguments[3]);
    }
    if constexpr (length == 5) {
        syscall5(stdout, arguments[0], arguments[1], arguments[2], arguments[3],
                 arguments[4]);
    }
}

auto write(i64 const& file_descriptor, Any p_string_buffer,
           usize const& string_length) -> Result<> {
    syscall(1, file_descriptor, p_string_buffer, string_length);
    return get_errno();
}
