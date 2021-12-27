// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <stdint.h>

// TODO: Optimize string_length().
// https://newbedev.com/why-does-glibc-s-strlen-need-to-be-so-complicated-to-run-quickly
// https://git.musl-libc.org/cgit/musl/tree/src/string/strlen.c

/* These are in the `std::` namespace so that they can be unambiguously
 * referenced inside of their `simd::` analogues. */
namespace std {

// NOLINTNEXTLINE
[[gnu::optimize("-fno-tree-loop-distribute-patterns")]] void copy_memory(
    void const* p_source, void* p_destination, isize bytes);

/* `T` is the return type of `string_length()`. It may be signed or unsigned.
 * There exists `simd::string_length_as<>()`. */
template <typename T>
constexpr auto string_length_as(char8_t const* p_string) -> T {
    T result = 0;
    while (p_string[result] != '\0') {
        result++;
    }
    return result;
}

auto is_aligned(void const volatile* pointer, isize byte_alignment) -> bool;

}  // namespace std

using std::copy_memory;
using std::is_aligned;
using std::string_length_as;

namespace simd {

// TODO: Power-of-two concept.
// template <isize Width>
// void copy_memory_small(void* p_destination, void const* p_source) {
//     static_assert(Width <= 256);
//     using Vector = std::detail::simd_vector<i1, Width>;
//     Vector source_vector = *reinterpret_cast<Vector const*>(p_source);
//     *reinterpret_cast<Vector*>(p_destination) = source_vector;
// }

void copy_memory(void const* p_source, void* p_destination, usize bytes);

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
        constexpr u1 mask =
            SIDD_UBYTE_OPS | SIDD_CMP_EQUAL_EACH | SIDD_LEAST_SIGNIFICANT;
        if (simd::cmp_implicit_str_c<mask>(data, zeroes)) {
            i1 const index = simd::cmp_implicit_str_i<mask>(data, zeroes);
            return result + index;
        }
        p_memory++;
        result += sizeof(u1x16);
        return result;
    }
    // This point unreachable because the function would segfault first.
    __builtin_unreachable();
}

}  // namespace simd

// These use primitive C types to be API compatible with libC.

// Deprecated call to `memcpy()`. Consider using `copy_buffer()` instead.
auto memcpy(void* p_destination, void const* p_source, size_t bytes) -> void*;
// Deprecated call to `strlen()`. Consider using `string_length_as<>()` instead.
auto strlen(char8_t const* p_string) -> size_t;
