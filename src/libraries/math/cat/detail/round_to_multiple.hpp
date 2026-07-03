// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/bit>
#include <cat/math>

namespace cat {

template <is_unsigned_integral T, is_unsigned_integral U>
[[nodiscard]]
constexpr auto
round_up_to_multiple_of(T value, U multiple) -> common_type<T, U> {
   using wide = common_type<T, U>;
   wide const wide_value = value;
   wide const wide_multiple = multiple;

   if (wide_multiple == 0u) {
      return wide_value;
   }

   // A power-of-two multiple rounds with a single mask instead of a modulo.
   if (has_single_bit(wide_multiple)) {
      return align_up(wide_value, wide_multiple);
   }

   wide const remainder = wide_value % wide_multiple;

   if (remainder == 0u) {
      return wide_value;
   }

   return static_cast<wide>(wide_value + (wide_multiple - remainder));
}

template <is_integral T, is_integral U>
[[nodiscard]]
constexpr auto
round_down_to_multiple_of(T value, U multiple) -> common_type<T, U> {
   using wide = common_type<T, U>;
   wide const wide_value = value;
   wide const wide_multiple = multiple;

   if (wide_multiple == 0) {
      return wide_value;
   }

   // A power-of-two multiple floors with a single mask instead of a modulo.
   if (has_single_bit(wide_multiple)) {
      return align_down(wide_value, wide_multiple);
   }

   wide const remainder = wide_value % wide_multiple;

   if (remainder == 0) {
      return wide_value;
   }

   // Floor toward negative infinity. `%` truncates toward zero, so a positive
   // remainder drops straight to the floor multiple while a negative remainder
   // sits one `multiple` above it and needs a further step down.
   if (remainder > 0) {
      return static_cast<wide>(wide_value - remainder);
   }
   return static_cast<wide>(wide_value - remainder - wide_multiple);
}

template <is_integral T, is_integral U>
[[nodiscard]]
constexpr auto
round_to_multiple_of(T value, U multiple) -> common_type<T, U> {
   using wide = common_type<T, U>;
   wide const wide_value = value;
   wide const wide_multiple = multiple;

   if (wide_multiple == 0) {
      return wide_value;
   }

   wide const down = round_down_to_multiple_of(wide_value, wide_multiple);
   if (down == wide_value) {
      return wide_value;
   }

   wide const up = static_cast<wide>(down + wide_multiple);

   // Pick the nearer multiple. A tie rounds toward positive infinity.
   if ((wide_value - down) >= (up - wide_value)) {
      return up;
   }
   return down;
}

}  // namespace cat
