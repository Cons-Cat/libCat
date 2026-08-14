// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>

namespace cat::detail {

template <is_raw_floating_point Float>
struct modulo_float_bits;

template <>
struct modulo_float_bits<float> {
   using uint_type = __UINT32_TYPE__;

   static constexpr int fraction_width = 23;
   static constexpr int exponent_width = 8;
};

template <>
struct modulo_float_bits<double> {
   using uint_type = __UINT64_TYPE__;

   static constexpr int fraction_width = 52;
   static constexpr int exponent_width = 11;
};

template <is_raw_floating_point Float>
[[nodiscard]]
constexpr auto
fmod_integer(Float left, Float right) -> Float {
   using traits = modulo_float_bits<Float>;
   using uint_type = traits::uint_type;
   constexpr int fraction_width = traits::fraction_width;
   constexpr int exponent_width = traits::exponent_width;
   constexpr uint_type sign_mask = uint_type(1)
                                   << (fraction_width + exponent_width);
   constexpr uint_type fraction_mask =
      (uint_type(1) << fraction_width) - uint_type(1);
   constexpr uint_type exponent_mask =
      ((uint_type(1) << exponent_width) - uint_type(1)) << fraction_width;

   uint_type left_bits = __builtin_bit_cast(uint_type, left);
   uint_type right_bits = __builtin_bit_cast(uint_type, right);
   uint_type const sign = left_bits & sign_mask;
   left_bits &= ~sign_mask;
   right_bits &= ~sign_mask;

   int left_exponent =
      static_cast<int>((left_bits & exponent_mask) >> fraction_width);
   int right_exponent =
      static_cast<int>((right_bits & exponent_mask) >> fraction_width);

   constexpr int special_exponent = (1 << exponent_width) - 1;
   if (
      (left_exponent == special_exponent && (left_bits & fraction_mask) != 0)
      || (right_exponent == special_exponent && (right_bits & fraction_mask) != 0)
   ) {
      return left + right;
   }
   if (right_bits == 0 || left_exponent == special_exponent) {
      Float const product = left * right;
      return product / product;
   }
   if (right_exponent == special_exponent) {
      return left;
   }
   if (left_bits <= right_bits) {
      if (left_bits == right_bits) {
         return __builtin_bit_cast(Float, sign);
      }
      return left;
   }

   if (left_exponent == 0) {
      while ((left_bits & (uint_type(1) << fraction_width)) == 0) {
         left_bits <<= 1;
         --left_exponent;
      }
   } else {
      left_bits =
         (left_bits & fraction_mask) | (uint_type(1) << fraction_width);
   }

   if (right_exponent == 0) {
      while ((right_bits & (uint_type(1) << fraction_width)) == 0) {
         right_bits <<= 1;
         --right_exponent;
      }
   } else {
      right_bits =
         (right_bits & fraction_mask) | (uint_type(1) << fraction_width);
   }

   while (left_exponent > right_exponent) {
      uint_type const difference = left_bits - right_bits;
      if (difference <= left_bits) {
         if (difference == 0) {
            return __builtin_bit_cast(Float, sign);
         }
         left_bits = difference;
      }
      left_bits <<= 1;
      --left_exponent;
   }

   uint_type const difference = left_bits - right_bits;
   if (difference <= left_bits) {
      if (difference == 0) {
         return __builtin_bit_cast(Float, sign);
      }
      left_bits = difference;
   }

   while ((left_bits & (uint_type(1) << fraction_width)) == 0) {
      left_bits <<= 1;
      --left_exponent;
   }

   if (left_exponent > 0) {
      left_bits = (left_bits & fraction_mask)
                  | (uint_type(left_exponent) << fraction_width);
   } else {
      left_bits >>= -left_exponent + 1;
   }
   return __builtin_bit_cast(Float, left_bits | sign);
}

}  // namespace cat::detail
