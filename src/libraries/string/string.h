// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

/* <legacy_string.h> is a compatibility library to make libC code compile with
 * libCat. It is not used directly in this header file. */
#include <legacy_string.h>
#include <type_traits>

// TODO: Optimize this.
/* Unlike strlen(), string_length returns a 32-bit type-safe signed integer,
 * because consistently using signed integers where reasonable produces
 * generally better codegen in several regards. */
constexpr auto string_length(char const* p_string) -> i32 {
    i32 result = 0;
    while (p_string[result] != '\0') {
        result++;
    }
    return result;
}

/* The SIMD version of string_length() returns a 64-bit integer, rather than a
 * 32-bit integer, because the overhead of SIMD would only be reasonable for
 * large strings (TODO: Prove that). This function requires SSE4.2 */
auto simd_string_length(char const* p_string) -> i64 {
    i64 result = 0;
    u8x16* p_memory = reinterpret_cast<u8x16*>(const_cast<char*>(p_string));
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
