// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// The `__builtin_memcpy()` triggers false diagnostics on high-optimization
// levels when link-time optimization is disabled.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"

template <typename T, typename U>
// Optimizing this function counter-intuitively seems to compile faster. It also
// inlines this function, resulting in a smaller debug binary.
[[gnu::always_inline,
#ifndef __OPTIMIZED__
  gnu::optimize(1),
#endif
  // Bit-casting a type can violate alignment assumptions, so that UBSan check
  // is disabled here.
  gnu::no_sanitize("alignment")]]
inline constexpr auto cat::bit_cast(U& from_value) -> T {
    if (__builtin_is_constant_evaluated()) {
        if constexpr (sizeof(U) == sizeof(T)) {
            // Fall back to C++20 bit-casting in a `constexpr` context.
            return __builtin_bit_cast(T, from_value);
        }
    } else {
        // TODO: Call `start_lifetime_as()` here.
        // This can be optimized into a bit-cast by GCC.
        T* p_to = static_cast<T*>(static_cast<void*>(
            const_cast<remove_const<U>*>(__builtin_addressof(from_value))));
        __builtin_memcpy(p_to, __builtin_addressof(from_value), sizeof(T));
        return *p_to;
    }
}

template <typename T, typename U>
// Optimizing this function counter-intuitively seems to compile faster. It also
// inlines this function, resulting in a smaller debug binary.
[[gnu::always_inline,
  // Bit-casting a type can violate alignment assumptions, so that UBSan check
  // is disabled here.
  gnu::no_sanitize("alignment")]]
inline constexpr auto cat::bit_cast(U const& from_value) -> T {
    return bit_cast<T>(unconst(from_value));
}

#pragma GCC diagnostic pop
