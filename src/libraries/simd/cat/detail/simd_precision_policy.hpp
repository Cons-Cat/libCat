// -*- mode: c++ -*-
#pragma once

#include <cat/arithmetic>

namespace cat::detail {

template <typename T>
inline constexpr precision_policies simd_float_precision_policy =
   precision_policies::precise;

template <typename T, precision_policies policy>
inline constexpr precision_policies
   simd_float_precision_policy<basic_float<T, policy>> = policy;

template <typename T, precision_policies precision>
struct simd_precision_scalar {
   using type = basic_float<T, precision>;
};

template <typename T, precision_policies old_precision,
          precision_policies precision>
struct simd_precision_scalar<basic_float<T, old_precision>, precision> {
   using type = basic_float<T, precision>;
};

template <typename T, typename RawVector>
[[nodiscard]]
constexpr auto
simd_reduce_policy_fadd(RawVector raw) -> raw_arithmetic_type<T> {
   if constexpr (simd_float_precision_policy<T> == precision_policies::fast) {
      return __builtin_reduce_assoc_fadd(raw);
   } else {
      return __builtin_reduce_in_order_fadd(raw, raw_arithmetic_type<T>{});
   }
}

template <typename T, typename RawVector>
[[nodiscard]]
constexpr auto
simd_reduce_policy_fadd(RawVector raw, T seed) -> raw_arithmetic_type<T> {
   raw_arithmetic_type<T> const raw_seed = make_raw_arithmetic(seed);
   if constexpr (simd_float_precision_policy<T> == precision_policies::fast) {
      return __builtin_reduce_assoc_fadd(raw, raw_seed);
   } else {
      return __builtin_reduce_in_order_fadd(raw, raw_seed);
   }
}

template <precision_policies precision, typename T, typename U>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_float_divide(T lhs, U rhs) {
   if constexpr (precision == precision_policies::precise) {
#pragma float_control(precise, on)
      return lhs / rhs;
   } else {
#pragma float_control(precise, off)
      return lhs / rhs;
   }
}

}  // namespace cat::detail
