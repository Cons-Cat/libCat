// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/emulated_fwd.hpp>

#include <cat/math>

namespace cat {
namespace detail {

// `sqrt`/`sqrtf` for targets without libM. Clang can lower lane
// `__builtin_sqrt`/`__builtin_sqrtf` and `__builtin_elementwise_sqrt` to.
// those C ABI symbols.
[[nodiscard]]
constexpr auto
emulated_sqrt(double argument) -> double {
   if (is_nan(argument)) {
      return argument;
   }
   if (argument == infinity) {
      return argument < 0. ? limits<double>::quiet_NaN() : argument;
   }
   if (argument == 0.) {
      return argument;
   }
   if (argument < 0.) {
      return limits<double>::quiet_NaN();
   }
   return emulated_exp(0.5 * emulated_log(argument));
}

[[nodiscard]]
constexpr auto
emulated_sqrtf(float argument) -> float {
   return static_cast<float>(emulated_sqrt(static_cast<double>(argument)));
}

}  // namespace detail

template <is_floating_point T>
   requires(arithmetic_quantity<T>::scale % 2 == 0)
[[nodiscard]]
constexpr auto
sqrt(T argument) {
   using raw_type = raw_arithmetic_type<T>;
   using result_quantity = powered_quantity<arithmetic_quantity<T>, 1, 2>;
   using result_type = detail::rebind_quantity_type<T, result_quantity{}>;
   raw_type raw_argument = make_raw_arithmetic(argument);

   if consteval {
      if constexpr (is_same<raw_type, float>) {
         return result_type(detail::emulated_sqrtf(raw_argument));
      } else {
         return result_type(detail::emulated_sqrt(raw_argument));
      }
   }
   return result_type(__builtin_elementwise_sqrt(raw_argument));
}

template <is_floating_point T>
   requires(arithmetic_quantity<T>::scale % 2 == 0)
[[nodiscard]]
constexpr auto
rsqrt(T argument) {
   using raw_type = raw_arithmetic_type<T>;
   using result_quantity = powered_quantity<arithmetic_quantity<T>, -1, 2>;
   using result_type = detail::rebind_quantity_type<T, result_quantity{}>;
   return result_type(raw_type(1) / make_raw_arithmetic(sqrt(argument)));
}

[[nodiscard, gnu::always_inline]]
constexpr auto
rsqrt(float4_fast argument) -> float4_fast {
   float const approx = __builtin_ia32_rsqrtss({argument.raw})[0];
   float const minus_half_x = -0.5f * argument.raw;
   float const three_halves = 1.5f;
   float const correction =
      __builtin_fmaf(minus_half_x, approx * approx, three_halves);
   return approx * correction;
}

}  // namespace cat
