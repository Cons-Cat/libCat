// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/math>

namespace cat {
namespace detail {

// `sqrt`/`sqrtf` for targets without libM. Clang can lower lane
// `__builtin_sqrt`/`__builtin_sqrtf` and `__builtin_elementwise_sqrt` to.
// those C ABI symbols. `sqrt(x)`: logarithmic identity `sqrt(x) = exp((1/2) ln
// x)` using the same Taylor/Maclaurin `exp` and Maclaurin `artanh` `log` path
// as above.
[[nodiscard]]
constexpr auto
emulated_sqrt(double argument) -> double {
   if (__builtin_isnan(argument)) {
      return argument;
   }
   if (__builtin_isinf(argument)) {
      return argument < 0. ? __builtin_nan("") : argument;
   }
   if (argument == 0.) {
      return argument;
   }
   if (argument < 0.) {
      return __builtin_nan("");
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

}  // namespace cat
