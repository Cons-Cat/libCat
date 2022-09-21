// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// TODO: This doesn't work with a `X* const` type.
template <typename T, typename U>
// Optimizing this function counter-intuitively seems to compile faster. It also
// inlines this function, resulting in a smaller debug binary.
[[gnu::always_inline,
#ifndef __OPTIMIZED__
  gnu::optimize("O1"),
#endif
  // Bit-casting a type can violate alignment assumptions, so that UBSan check
  // is disabled here.
  gnu::no_sanitize("alignment")]] constexpr inline auto
cat::bit_cast(U& from_value) -> T {
    if (__builtin_is_constant_evaluated()) {
        if constexpr (sizeof(from_value) == sizeof(T)) {
            // Fall back to C++20 bit-casting in a `constexpr` context.
            return __builtin_bit_cast(T, from_value);
        }
    } else {
        // This can be optimized into a bit-cast by GCC.
        T* p_to = static_cast<T*>(static_cast<void*>(
            const_cast<RemoveConst<U>*>(__builtin_addressof(from_value))));
        __builtin_memcpy(p_to, __builtin_addressof(from_value), sizeof(T));
        return *p_to;
    }
}
