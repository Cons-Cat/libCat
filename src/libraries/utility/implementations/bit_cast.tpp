// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/meta>
#include <cat/utility>

/* As far as I can prove, `bit_cast` is a zero-overhead abstraction on `-O3`.
 * It copies data at some memory byte-by-byte into a differently-typed variable
 * at its own address. Compilers are good at optimizing out this pattern. Four
 * overloads are required, which I believe is still simpler than catprogramming
 * one `bit_cast` function. A `void*`, `void* const`, `T`, and `T const` each
 * resolve to different overloads. These are optimized and inlined because it is
 * sometimes semantically necessary that these have zero-overhead, and those
 * attributes guarantee this even in -O0. */

// TODO: Add an overload for C++20 bit casting.

// `cat::bit_cast` should always be optimized. Otherwise, the compiler will not
// streamline it out, which sometimes causes undefined behavior due to
// pointer-misalignment.
#pragma GCC optimize("-O3")

template <typename T>
[[gnu::always_inline]] constexpr inline auto cat::bit_cast(auto& from_value)
    -> T {
    /* Cast the address of `from_value` into a pointer of its type (with a
     * possible reference removed), then remove its possible `const` qualifier.
     * Cast that to `void*`, then cast that to `char*`, which represents bytes.
     */
    char* p_from = static_cast<char*>(static_cast<void*>(
        const_cast<
            cat::RemoveConst<cat::RemoveReference<decltype(from_value)>>*>(
            &from_value)));
    T* p_to = static_cast<T*>(static_cast<void*>(p_from));

    // GCC optimizes this pattern into a bit-cast:
    for (unsigned i = 0; i < sizeof(T); i++) {
        static_cast<char*>(
            const_cast<void*>(static_cast<void const*>(p_to)))[i] = p_from[i];
    }
    return *p_to;
}
