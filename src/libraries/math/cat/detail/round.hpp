// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>

namespace cat {

template <is_integral T>
[[nodiscard]]
constexpr auto
floor(T value) -> T {
   return value;
}

template <is_floating_point Float>
[[nodiscard]]
constexpr auto
floor(Float value) -> Float {
   using raw_type = raw_arithmetic_type<Float>;
   raw_type const raw_value = make_raw_arithmetic(value);
   if consteval {
      if (!is_finite(raw_value) || raw_value == 0.f) {
         return value;
      }
      constexpr raw_type exact_integer_limit =
         is_same<raw_type, float> ? 0x1p24f : 0x1p53;
      if (
         raw_value >= exact_integer_limit || raw_value <= -exact_integer_limit
      ) {
         return value;
      }
      raw_type const integral =
         static_cast<raw_type>(static_cast<long long>(raw_value));
      return integral > raw_value ? integral - 1.f : integral;
   }
   return Float(__builtin_elementwise_floor(raw_value));
}

template <is_integral T>
[[nodiscard]]
constexpr auto
ceil(T value) -> T {
   return value;
}

template <is_floating_point Float>
[[nodiscard]]
constexpr auto
ceil(Float value) -> Float {
   using raw_type = raw_arithmetic_type<Float>;
   raw_type const raw_value = make_raw_arithmetic(value);
   if consteval {
      if (!is_finite(raw_value) || raw_value == 0) {
         return value;
      }
      constexpr raw_type exact_integer_limit =
         is_same<raw_type, float> ? 0x1p24f : 0x1p53;
      if (
         raw_value >= exact_integer_limit || raw_value <= -exact_integer_limit
      ) {
         return value;
      }
      raw_type const integral =
         static_cast<raw_type>(static_cast<long long>(raw_value));
      if (integral < raw_value) {
         return integral + 1.f;
      }
      return integral == 0 && raw_value < 0 ? -0.f : integral;
   }
   return Float(__builtin_elementwise_ceil(raw_value));
}

}  // namespace cat
