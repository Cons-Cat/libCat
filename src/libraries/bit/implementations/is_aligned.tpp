// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/bit>

// Returns `true` if `p_value` is aligned to the `alignment` boundary.

namespace cat::detail {

template <typename PointerInteger>
[[nodiscard]]
constexpr auto
is_basic_intptr_aligned(PointerInteger p_value, uword alignment) -> bool {
   using raw_type = PointerInteger::raw_type;
   using unsigned_raw_type = make_unsigned_type<raw_type>;
   unsigned_raw_type const raw = static_cast<unsigned_raw_type>(p_value.raw);
   unsigned_raw_type const mask =
      static_cast<unsigned_raw_type>((alignment - 1u).raw);
   return (raw & mask) == 0u;
}

}  // namespace cat::detail

namespace cat {

template <typename U>
[[nodiscard]]
constexpr bool
is_aligned(U* _Nullable p_value, uword alignment)
   __attribute__((enable_if(has_single_bit(alignment),
                            "`alignment` must be a power of two!"))) {
   uintptr<U> const p_integer{__builtin_bit_cast(__UINTPTR_TYPE__, p_value)};
   return detail::is_basic_intptr_aligned(p_integer, alignment);
}

template <typename U>
[[nodiscard]]
constexpr bool
is_aligned(U* _Nullable, uword alignment)
   __attribute__((enable_if(!has_single_bit(alignment),
                            "`alignment` must be a power of two!"))) = delete;

template <typename U>
[[nodiscard]]
constexpr auto
is_aligned(U* _Nullable p_value, uword alignment) -> bool {
   uintptr<U> const p_integer{__builtin_bit_cast(__UINTPTR_TYPE__, p_value)};
   return detail::is_basic_intptr_aligned(p_integer, alignment);
}

// Returns `true` if `p_value` is aligned to the `alignment` boundary.
template <typename U, typename Storage, overflow_policies policy>
[[nodiscard]]
constexpr bool
is_aligned(basic_intptr<U, Storage, policy> p_value, uword alignment)
   __attribute__((enable_if(has_single_bit(alignment),
                            "`alignment` must be a power of two!"))) {
   return detail::is_basic_intptr_aligned(p_value, alignment);
}

template <typename U, typename Storage, overflow_policies policy>
[[nodiscard]]
constexpr bool
is_aligned(basic_intptr<U, Storage, policy>, uword alignment)
   __attribute__((enable_if(!has_single_bit(alignment),
                            "`alignment` must be a power of two!"))) = delete;

template <typename U, typename Storage, overflow_policies policy>
[[nodiscard]]
constexpr auto
is_aligned(basic_intptr<U, Storage, policy> p_value, uword alignment) -> bool {
   return detail::is_basic_intptr_aligned(p_value, alignment);
}

// Returns `true` if `value` is aligned to the `alignment` boundary.
template <cat::overflow_policies policy>
[[nodiscard]]
constexpr auto
is_aligned(cat::basic_idx<policy> value, cat::uword alignment) -> bool {
   return (uword(value) & (alignment - 1u)) == 0u;
}

}  // namespace cat
