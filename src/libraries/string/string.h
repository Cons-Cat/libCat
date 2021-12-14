// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

/* <legacy_string.h> is a compatibility library to make libC code compile with
 * libCat. It is not used directly in this header file. */
#include <legacy_string.h>

// TODO: Optimize string_length().
// https://newbedev.com/why-does-glibc-s-strlen-need-to-be-so-complicated-to-run-quickly
// https://git.musl-libc.org/cgit/musl/tree/src/string/strlen.c

/* T is the return type of string_length(). It may be signed or unsigned. */
template <typename T>
constexpr auto string_length(char8_t const* p_string) -> T {
    T result = 0;
    while (p_string[result] != '\0') {
        result++;
    }
    return result;
}

[[deprecated("strlen() is deprecated! Use string_length<T>() instead.")]] auto
strlen(char8_t const* p_string) -> size_t {
    string_length<size_t>(p_string);
}

/* T is the return type of simd_string_length(). It may be signed or unsigned.
 * This function requires SSE4.2 */
template <typename T>
auto simd_string_length(u8 const* p_string) -> T {
    T result = 0;
    u8x16* p_memory = reinterpret_cast<u8x16*>(const_cast<u8*>(p_string));
    u8x16 zeros = simd_setzero<u8x16>();
    while (true) {
        u8x16 data = simd_load(p_memory);
        constexpr u8 mask =
            ::SIDD_UBYTE_OPS | ::SIDD_CMP_EQUAL_EACH | ::SIDD_LEAST_SIGNIFICANT;
        if (simd_cmp_implicit_str_c<mask>(data, zeros)) {
            i64 const index = simd_cmp_implicit_str_i<mask>(data, zeros);
            return result + index;
        }
        p_memory++;
        result += sizeof(u8x16);
    }
    /* This point is impossible to reach, because the function would segfault
     * first. */
    __builtin_unreachable();
}
