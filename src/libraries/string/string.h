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
constexpr auto string_length_as(char8_t const* p_string) -> T {
    T result = 0;
    while (p_string[result] != '\0') {
        result++;
    }
    return result;
}

[[deprecated("strlen() is deprecated! Use string_length<T>() instead.")]] auto
strlen(char8_t const* p_string) -> size_t {
    return string_length_as<size_t>(p_string);
}

void copy_memory(void* source, void* destination, usize bytes) {
    if (bytes <= 16) {
        // __builtin_memcpy(source, destination, bytes);
    }
}

// TODO: Move into a <bit.h> library.
auto is_aligned(void const volatile* pointer, usize byte_alignment) -> bool {
    return (reinterpret_cast<usize>(pointer) % byte_alignment) == 0;
}

namespace simd {

/* T is the return type of string_length_as(). It may be signed or
 * unsigned. This function requires SSE4.2 */
template <typename T>
auto string_length_as(char8_t const* p_string) -> T {
    // TODO: Align pointers.
    T result = 0;
    charx16* p_memory = p_string_to_p_vector<16>(p_string);
    constexpr charx16 zeroes = simd::set_zeros<charx16>();

    while (true) {
        charx16 data;
        data = *p_memory;
        constexpr u8 mask =
            SIDD_UBYTE_OPS | SIDD_CMP_EQUAL_EACH | SIDD_LEAST_SIGNIFICANT;
        if (simd::cmp_implicit_str_c<mask>(data, zeroes)) {
            i64 const index = simd::cmp_implicit_str_i<mask>(data, zeroes);
            return result + index;
        }
        p_memory++;
        result += sizeof(u8x16);
        return result;
    }
    // This point unreachable because the function would segfault first.
    __builtin_unreachable();
}

}  // namespace simd
