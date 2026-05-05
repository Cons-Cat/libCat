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

#pragma float_control(push)
#pragma float_control(precise, on)

template <typename RawVector>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
precise_simd_float_equal(RawVector left, RawVector right) {
   return left == right;
}

template <typename RawVector>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
precise_simd_float_unequal(RawVector left, RawVector right) {
   return left != right;
}

template <typename RawVector>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
precise_simd_float_less(RawVector left, RawVector right) {
   return left < right;
}

template <typename RawVector>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
precise_simd_float_less_equal(RawVector left, RawVector right) {
   return left <= right;
}

template <typename RawVector>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
precise_simd_float_greater(RawVector left, RawVector right) {
   return left > right;
}

template <typename RawVector>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
precise_simd_float_greater_equal(RawVector left, RawVector right) {
   return left >= right;
}

template <typename RawVector>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
precise_simd_float_fma(RawVector left, RawVector right, RawVector addend) {
   return __builtin_elementwise_fma(left, right, addend);
}

#pragma float_control(pop)

#pragma float_control(push)
#pragma float_control(precise, off)

template <typename RawVector>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
fast_simd_float_equal(RawVector left, RawVector right) {
   return left == right;
}

template <typename RawVector>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
fast_simd_float_unequal(RawVector left, RawVector right) {
   return left != right;
}

template <typename RawVector>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
fast_simd_float_less(RawVector left, RawVector right) {
   return left < right;
}

template <typename RawVector>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
fast_simd_float_less_equal(RawVector left, RawVector right) {
   return left <= right;
}

template <typename RawVector>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
fast_simd_float_greater(RawVector left, RawVector right) {
   return left > right;
}

template <typename RawVector>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
fast_simd_float_greater_equal(RawVector left, RawVector right) {
   return left >= right;
}

template <typename RawVector>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
fast_simd_float_fma(RawVector left, RawVector right, RawVector addend) {
   return __builtin_elementwise_fma(left, right, addend);
}

#pragma float_control(pop)

template <precision_policies precision, typename RawVector>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_float_equal(RawVector left, RawVector right) {
   if constexpr (precision == precision_policies::precise) {
      return precise_simd_float_equal(left, right);
   } else {
      return fast_simd_float_equal(left, right);
   }
}

template <precision_policies precision, typename RawVector>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_float_unequal(RawVector left, RawVector right) {
   if constexpr (precision == precision_policies::precise) {
      return precise_simd_float_unequal(left, right);
   } else {
      return fast_simd_float_unequal(left, right);
   }
}

template <precision_policies precision, typename RawVector>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_float_less(RawVector left, RawVector right) {
   if constexpr (precision == precision_policies::precise) {
      return precise_simd_float_less(left, right);
   } else {
      return fast_simd_float_less(left, right);
   }
}

template <precision_policies precision, typename RawVector>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_float_less_equal(RawVector left, RawVector right) {
   if constexpr (precision == precision_policies::precise) {
      return precise_simd_float_less_equal(left, right);
   } else {
      return fast_simd_float_less_equal(left, right);
   }
}

template <precision_policies precision, typename RawVector>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_float_greater(RawVector left, RawVector right) {
   if constexpr (precision == precision_policies::precise) {
      return precise_simd_float_greater(left, right);
   } else {
      return fast_simd_float_greater(left, right);
   }
}

template <precision_policies precision, typename RawVector>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_float_greater_equal(RawVector left, RawVector right) {
   if constexpr (precision == precision_policies::precise) {
      return precise_simd_float_greater_equal(left, right);
   } else {
      return fast_simd_float_greater_equal(left, right);
   }
}

template <precision_policies precision, typename RawVector>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_float_fma(RawVector left, RawVector right, RawVector addend) {
   if constexpr (precision == precision_policies::precise) {
      return precise_simd_float_fma(left, right, addend);
   } else {
      return fast_simd_float_fma(left, right, addend);
   }
}

}  // namespace cat::detail
