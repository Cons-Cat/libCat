// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/math>

namespace cat {

template <is_floating_point T>
[[nodiscard]]
constexpr auto
nroot(T x, iword n) -> T {
   if (n <= 1u) {
      return x;
   }
   if (x == 0) {
      return x;
   }

   T const abs_x = (x < 0u) ? -x : x;
   T const inv_n = T(1) / T(n);
   T const root_magnitude = cat::pow(abs_x, inv_n);

   if (x < 0u) {
      if ((n.raw & 1u) != 0u) {
         return -root_magnitude;
      }
      if constexpr (is_same<raw_arithmetic_type<T>, float>) {
         return T(__builtin_nanf(""));
      } else {
         return T(__builtin_nan(""));
      }
   }

   return root_magnitude;
}

template <is_floating_point T>
[[nodiscard]]
constexpr auto
cbrt(T x) -> T {
   return cat::nroot(x, 3);
}

template <is_floating_point T>
[[nodiscard]]
constexpr auto
rnroot(T x, iword n) -> T {
   if (n <= 0) {
      return x;
   }
   if (n == 1) {
      return T(1) / x;
   }
   return T(1) / cat::nroot(x, n);
}

template <is_floating_point T>
[[nodiscard]]
constexpr auto
rcbrt(T x) -> T {
   return cat::rnroot(x, 3);
}

}  // namespace cat
