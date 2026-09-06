// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/math>

namespace cat {

template <is_floating_point T>
   requires is_dimensionless_arithmetic<T>
[[nodiscard]]
constexpr auto
nroot(T x, iword n) -> T;

template <int root, is_floating_point T>
   requires(root != 0 && arithmetic_quantity<T>::scale % root == 0)
[[nodiscard]]
constexpr auto
nroot(T x) {
   using result_quantity = powered_quantity<arithmetic_quantity<T>, 1, root>;
   using result_type = detail::rebind_quantity_type<T, result_quantity{}>;
   return result_type(cat::nroot(make_raw_arithmetic(x), root));
}

template <is_floating_point T>
   requires is_dimensionless_arithmetic<T>
[[nodiscard]]
constexpr auto
nroot(T x, iword n) -> T {
   using raw_type = raw_arithmetic_type<T>;
   if (n <= 1u) {
      return x;
   }
   raw_type const raw_x = make_raw_arithmetic(x);
   if (raw_x == 0) {
      return x;
   }

   raw_type const abs_x = raw_x < 0 ? -raw_x : raw_x;
   raw_type const inv_n = raw_type(1) / static_cast<raw_type>(n);
   raw_type const root_magnitude = cat::pow(abs_x, inv_n);

   if (raw_x < 0) {
      if ((n.raw & 1u) != 0) {
         return T(-root_magnitude);
      }
      return limits<T>::quiet_NaN();
   }

   return T(root_magnitude);
}

template <is_floating_point T>
   requires(arithmetic_quantity<T>::scale % 3 == 0)
[[nodiscard]]
constexpr auto
cbrt(T x) {
   return cat::nroot<3>(x);
}

template <is_floating_point T>
   requires is_dimensionless_arithmetic<T>
[[nodiscard]]
constexpr auto
rnroot(T x, iword n) -> T;

template <int root, is_floating_point T>
   requires(root != 0 && arithmetic_quantity<T>::scale % root == 0)
[[nodiscard]]
constexpr auto
rnroot(T x) {
   using result_quantity = powered_quantity<arithmetic_quantity<T>, -1, root>;
   using result_type = detail::rebind_quantity_type<T, result_quantity{}>;
   return result_type(cat::rnroot(make_raw_arithmetic(x), root));
}

template <is_floating_point T>
   requires is_dimensionless_arithmetic<T>
[[nodiscard]]
constexpr auto
rnroot(T x, iword n) -> T {
   using raw_type = raw_arithmetic_type<T>;
   if (n <= 0) {
      return x;
   }
   if (n == 1) {
      return T(raw_type(1) / make_raw_arithmetic(x));
   }
   return T(raw_type(1) / make_raw_arithmetic(cat::nroot(x, n)));
}

template <is_floating_point T>
   requires(arithmetic_quantity<T>::scale % 3 == 0)
[[nodiscard]]
constexpr auto
rcbrt(T x) {
   return cat::rnroot<3>(x);
}

}  // namespace cat
