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

void copy_memory(void* p_source, void* p_destination, isize bytes) {
    char const* source_iterator = static_cast<char*>(p_source);
    char* destination_iterator = static_cast<char*>(p_destination);
    while (bytes > 0) {
        *destination_iterator++ = *source_iterator++;
        bytes--;
    }
}

// TODO: Move into a <bit.h> library.
auto is_aligned(void const volatile* pointer, isize byte_alignment) -> bool {
    return (reinterpret_cast<usize>(pointer) % byte_alignment) == 0;
}

namespace simd {

// TODO: Power-of-two concept.
template <isize Width>
void copy_memory_small(void* p_destination, void const* p_source) {
    static_assert(Width <= 256);
    using Vector = std::detail::simd_vector<i1, Width>;
    Vector source_vector = *reinterpret_cast<Vector const*>(p_source);
    *reinterpret_cast<Vector*>(p_destination) = source_vector;
}

// TODO: Use Any instead of chars.
void copy_memory(void* p_destination, void const* p_source, isize size) {
    using Vector = std::detail::simd_vector<long long int, 4>;
    unsigned char* dst = reinterpret_cast<unsigned char*>(p_destination);
    unsigned char const* src = reinterpret_cast<unsigned char const*>(p_source);
    static isize cachesize = 0x200000;  // L3-cache size.
    isize padding;

    if (size <= 256) {
        // TODO: Fill this in.
        simd::zero_upper_avx_registers();
    }

    // Align src, dst, and size to 16 bytes
    padding = (32 - ((reinterpret_cast<isize>(dst)) & 31)) & 31;

    Vector head = *reinterpret_cast<Vector const*>(p_source);
    *static_cast<Vector*>(p_destination) = head;
    src += padding;
    dst += padding;
    size -= padding;

    Vector vectors[8];
    // This routine is optimized for buffers in L3 cache. Streaming is slower.
    if (size <= cachesize) {
        while (size >= 256) {
            // TODO: Unroll the two loops in this scope?
            for (i4 i = 0; i < 8; i++) {
                vectors[i] = *(
                    const_cast<Vector*>(reinterpret_cast<Vector const*>(src)) +
                    i);
            }
            prefetch(src + 512, simd::MM_HINT_NTA);
            src += 256;
            for (i4 i = 0; i < 8; i++) {
                *(reinterpret_cast<Vector*>(dst)) = vectors[i];
            }
            dst += 256;
            size -= 256;
        }
    } else {
        prefetch(src + 512, simd::MM_HINT_NTA);
        /* TODO: This could be improved by using aligned-streaming when
         * possible. */
        while (size >= 256) {
            // TODO: Unroll the two loops in this scope?
            for (i4 i = 0; i < 8; i++) {
                vectors[i] = *(
                    const_cast<Vector*>(reinterpret_cast<Vector const*>(src)) +
                    i);
            }
            prefetch(src + 512, simd::MM_HINT_NTA);
            src += 256;
            for (i4 i = 0; i < 8; i++) {
                stream_in(dst, &vectors[i]);
            }
            dst += 256;
            size -= 256;
        }
        simd::fence();
    }
    simd::zero_upper_avx_registers();
}

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

[[deprecated("memcpy() is deprecated! Use simd::copy_buffer() instead!")]] auto
memcpy(void* p_destination, void const* p_source, size_t bytes) -> void* {
    simd::copy_memory(p_destination, p_source, bytes);
    return p_destination;
}

[[deprecated(
    "strlen() is deprecated! Use simd::string_length<T>() instead.")]] auto
strlen(char8_t const* p_string) -> size_t {
    return simd::string_length_as<size_t>(p_string);
}
