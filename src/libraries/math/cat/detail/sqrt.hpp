// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

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
[[nodiscard]]
constexpr auto
sqrt(T argument) -> T {
   using raw_type = raw_arithmetic_type<T>;
   raw_type raw_argument = make_raw_arithmetic(argument);

   return T(__builtin_elementwise_sqrt(raw_argument));
}

template <is_floating_point T>
[[nodiscard]]
constexpr auto
rsqrt(T argument) -> T {
   return T(1) / sqrt(argument);
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
