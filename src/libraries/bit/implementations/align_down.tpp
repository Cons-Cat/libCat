// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/bit>

// Returns a value rounded down from `p_value` to the nearest `alignment`
// boundary.

namespace cat::detail {

template <typename PointerInteger>
[[nodiscard]]
constexpr auto
align_down_basic_intptr(PointerInteger p_value, uword alignment)
   -> PointerInteger {
   using raw_type = PointerInteger::raw_type;
   using unsigned_raw_type = make_unsigned_type<raw_type>;
   unsigned_raw_type const raw = static_cast<unsigned_raw_type>(p_value.raw);
   unsigned_raw_type const mask =
      static_cast<unsigned_raw_type>(-alignment.raw);
   return PointerInteger(__builtin_bit_cast(raw_type, raw & mask));
}

}  // namespace cat::detail

namespace cat {

template <typename U>
[[nodiscard]]
constexpr U* _Nullable align_down(U* _Nullable p_value, uword alignment)
   __attribute__((enable_if(has_single_bit(alignment),
                            "`alignment` must be a power of two!"))) {
   uintptr<U> const p_integer{__builtin_bit_cast(__UINTPTR_TYPE__, p_value)};
   return __builtin_bit_cast(
      U*, detail::align_down_basic_intptr(p_integer, alignment).raw);
}

template <typename U>
[[nodiscard]]
constexpr U* _Nullable align_down(U* _Nullable, uword alignment)
   __attribute__((enable_if(!has_single_bit(alignment),
                            "`alignment` must be a power of two!"))) = delete;

template <typename U>
[[nodiscard]]
constexpr auto
align_down(U* _Nullable p_value, uword alignment) -> U* _Nullable {
   uintptr<U> const p_integer{__builtin_bit_cast(__UINTPTR_TYPE__, p_value)};
   return __builtin_bit_cast(
      U*, detail::align_down_basic_intptr(p_integer, alignment).raw);
}

// Returns a value rounded down from `p_value` to the nearest `alignment`
// boundary. This only works for two's complement arithmetic.
template <typename U, typename Storage, overflow_policies policy>
[[nodiscard]]
constexpr basic_intptr<U, Storage, policy>
align_down(basic_intptr<U, Storage, policy> p_value, uword alignment)
   __attribute__((enable_if(has_single_bit(alignment),
                            "`alignment` must be a power of two!"))) {
   return detail::align_down_basic_intptr(p_value, alignment);
}

template <typename U, typename Storage, overflow_policies policy>
[[nodiscard]]
constexpr basic_intptr<U, Storage, policy>
align_down(basic_intptr<U, Storage, policy>, uword alignment)
   __attribute__((enable_if(!has_single_bit(alignment),
                            "`alignment` must be a power of two!"))) = delete;

template <typename U, typename Storage, overflow_policies policy>
[[nodiscard]]
constexpr auto
align_down(basic_intptr<U, Storage, policy> p_value, uword alignment)
   -> basic_intptr<U, Storage, policy> {
   return detail::align_down_basic_intptr(p_value, alignment);
}

}  // namespace cat
