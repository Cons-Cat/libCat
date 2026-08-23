// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/meta>
#include <cat/simd>
#include <cat/simd_ops>

namespace cat::detail {

template <is_unsigned_integral T, typename Next>
constexpr auto
lemire_bounded(T bound, Next&& next) -> T {
   using raw_type = raw_arithmetic_type<T>;
   raw_type const limit = make_raw_arithmetic(bound);
   raw_type raw = make_raw_arithmetic(next());
   __uint128_t sample = __uint128_t(raw) * __uint128_t(limit);
   uword const shift = sizeof(T) * 8u;
   raw_type fraction = raw_type(sample);
   raw_type high = raw_type(sample >> shift);
   if (fraction < limit) {
      raw_type const threshold = raw_type(raw_type(0u) - limit) % limit;
      while (fraction < threshold) {
         raw = make_raw_arithmetic(next());
         sample = __uint128_t(raw) * __uint128_t(limit);
         fraction = raw_type(sample);
         high = raw_type(sample >> shift);
      }
   }
   return T(high);
}

template <is_simd_unsigned_integral T>
   requires(sizeof(typename T::value_type) <= 8u)
constexpr auto
lemire_multiply_high(T left, T right) -> T {
   using lane_type = T::value_type;
   constexpr lane_type half_bits = sizeof(lane_type) * 4u;
   constexpr lane_type half_mask = (lane_type(1u) << half_bits) - lane_type(1u);
   T const left_low = left & half_mask;
   T const left_high = left >> half_bits;
   T const right_low = right & half_mask;
   T const right_high = right >> half_bits;
   T const low_product = left_low * right_low;
   T const middle = left_high * right_low + (low_product >> half_bits);
   T const middle_low = middle & half_mask;
   T const middle_high = middle >> half_bits;
   T const cross = middle_low + left_low * right_high;
   return left_high * right_high + middle_high + (cross >> half_bits);
}

template <is_simd_unsigned_integral T, typename Next>
   requires(sizeof(typename T::value_type) <= 8u)
constexpr auto
lemire_bounded(T bound, Next&& next) -> T {
   using mask_type = T::mask_type;
   T const zero = 0u;
   mask_type const unbounded = bound.equal_lanes(zero);
   T const divisor = simd_select(unbounded, T(1u), bound);
   T threshold = 0u;
   bool threshold_ready = false;
   T result = 0u;
   mask_type pending(true);
   while (pending.any_of()) {
      T const value = [&] {
         if constexpr (requires { next(pending); }) {
            return next(pending);
         } else {
            return next();
         }
      }();
      T const low = value * divisor;
      T const high = lemire_multiply_high(value, divisor);
      mask_type const slow = pending & !unbounded & (low < divisor);
      mask_type accepted = pending & !slow;
      if (slow.any_of()) {
         if (!threshold_ready) {
            threshold = (zero - divisor) % divisor;
            threshold_ready = true;
         }
         accepted |= slow & (low >= threshold);
      }
      result =
         simd_select(accepted, simd_select(unbounded, value, high), result);
      pending &= !accepted;
   }
   return result;
}

}  // namespace cat::detail
