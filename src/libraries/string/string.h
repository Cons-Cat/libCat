// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// TODO: Optimize string_length().
// https://newbedev.com/why-does-glibc-s-strlen-need-to-be-so-complicated-to-run-quickly
// https://git.musl-libc.org/cgit/musl/tree/src/string/strlen.c

/* These are in the `std::` namespace so that they can be unambiguously
 * referenced inside of their `simd::` analogues. */
namespace std {

// NOLINTNEXTLINE
[[gnu::optimize("-fno-tree-loop-distribute-patterns")]] void copy_memory(
    void const* p_source, void* p_destination, isize bytes);

template <typename T>
constexpr auto string_length_as(char8_t const* p_string) -> T;

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

template <typename T>
auto string_length_as(char8_t const* p_string) -> T;

void copy_memory(void const* p_source, void* p_destination, usize bytes);

}  // namespace simd

// These use primitive C types to be API compatible with libC.

// Deprecated call to `memcpy()`. Consider using `copy_buffer()` instead.
auto memcpy(void* p_destination, void const* p_source, size_t bytes) -> void*;
// Deprecated call to `strlen()`. Consider using `string_length_as<>()` instead.
auto strlen(char8_t const* p_string) -> size_t;

#include "./impl/string_length_as.tpp"
