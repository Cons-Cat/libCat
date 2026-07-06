// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/simd_precision_policy.hpp>

#include <cat/arithmetic>

namespace cat::detail {

template <typename T, typename Abi>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_mul_sat_lanes(
   cat::simd<T, Abi> const& left, cat::simd<T, Abi> const& right
) -> cat::simd<T, Abi> {
   cat::simd<T, Abi> out{};
   for (idx i = 0u; i < left.size(); ++i) {
      out.set_lane(i, left[i] * right[i]);
   }
   return out;
}

template <typename T, typename Abi, typename F>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_shift_lanes(
   F&& shift_fn, cat::simd<T, Abi> const& left, cat::simd<T, Abi> const& right
) -> cat::simd<T, Abi> {
   cat::simd<T, Abi> out{};
   for (idx i = 0u; i < left.size(); ++i) {
      out.set_lane(i, shift_fn(left[i], right[i]));
   }
   return out;
}

template <typename T, typename Abi, typename F>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_lanewise_op(
   F&& op, cat::simd<T, Abi> const& left, cat::simd<T, Abi> const& right
) -> cat::simd<T, Abi> {
   cat::simd<T, Abi> out{};
   for (idx i = 0u; i < left.size(); ++i) {
      out.set_lane(i, op(left[i], right[i]));
   }
   return out;
}

// Reinterpret the raw vector as its unsigned-lane twin. The bit layout is
// identical, so this is a pure bitcast wrapping arithmetic on the unsigned
// twin avoids the signed-overflow UB the C++ standard would assign to a
// signed `+`, `-`, `*`, or `<<`.
template <typename T, typename Abi>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_to_unsigned_raw(cat::simd<T, Abi> const& value) {
   using raw_scalar = raw_arithmetic_type<T>;
   using unsigned_scalar = make_unsigned_type<raw_scalar>;
   using unsigned_raw =
      unsigned_scalar __attribute__((vector_size(sizeof(value.raw))));
   return __builtin_bit_cast(unsigned_raw, value.raw);
}

template <typename T, typename Abi, typename UnsignedRaw>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_from_unsigned_raw(UnsignedRaw const& value) -> cat::simd<T, Abi> {
   using raw_type = typename cat::simd<T, Abi>::raw_type;
   return cat::simd<T, Abi>(__builtin_bit_cast(raw_type, value));
}

template <typename T, typename Abi, overflow_policies policy>
   requires(is_integral<T> && !is_bool<T>)
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_add_policy(
   cat::simd<T, Abi> const& left, cat::simd<T, Abi> const& right
) -> cat::simd<T, Abi> {
   if constexpr (policy == overflow_policies::saturate) {
      return cat::simd<T, Abi>(
         __builtin_elementwise_add_sat(left.raw, right.raw)
      );
   } else if constexpr (
      policy == overflow_policies::wrap && is_signed<raw_arithmetic_type<T>>
   ) {
      return simd_from_unsigned_raw<T, Abi>(
         simd_to_unsigned_raw(left) + simd_to_unsigned_raw(right)
      );
   } else {
      return cat::simd<T, Abi>(left.raw + right.raw);
   }
}

template <typename T, typename Abi, overflow_policies policy>
   requires(is_integral<T> && !is_bool<T>)
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_sub_policy(
   cat::simd<T, Abi> const& left, cat::simd<T, Abi> const& right
) -> cat::simd<T, Abi> {
   if constexpr (policy == overflow_policies::saturate) {
      return cat::simd<T, Abi>(
         __builtin_elementwise_sub_sat(left.raw, right.raw)
      );
   } else if constexpr (
      policy == overflow_policies::wrap && is_signed<raw_arithmetic_type<T>>
   ) {
      return simd_from_unsigned_raw<T, Abi>(
         simd_to_unsigned_raw(left) - simd_to_unsigned_raw(right)
      );
   } else {
      return cat::simd<T, Abi>(left.raw - right.raw);
   }
}

template <typename T, typename Abi, overflow_policies policy>
   requires(is_integral<T> && !is_bool<T>)
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_mul_policy(
   cat::simd<T, Abi> const& left, cat::simd<T, Abi> const& right
) -> cat::simd<T, Abi> {
   if constexpr (policy == overflow_policies::saturate) {
      return simd_integral_mul_sat_lanes(left, right);
   } else if constexpr (
      policy == overflow_policies::wrap && is_signed<raw_arithmetic_type<T>>
   ) {
      return simd_from_unsigned_raw<T, Abi>(
         simd_to_unsigned_raw(left) * simd_to_unsigned_raw(right)
      );
   } else {
      return cat::simd<T, Abi>(left.raw * right.raw);
   }
}

template <typename T, typename Abi, overflow_policies policy>
   requires(is_integral<T> && !is_bool<T>)
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_div_policy(
   cat::simd<T, Abi> const& left, cat::simd<T, Abi> const& right
) -> cat::simd<T, Abi> {
   if constexpr (
      policy == overflow_policies::saturate && is_signed<raw_arithmetic_type<T>>
   ) {
      return simd_integral_lanewise_op<T, Abi>(
         [](T l, T r) {
            return T(sat_div(make_raw_arithmetic(l), make_raw_arithmetic(r)));
         },
         left, right
      );
   } else if constexpr (
      policy == overflow_policies::wrap && is_signed<raw_arithmetic_type<T>>
   ) {
      return simd_integral_lanewise_op<T, Abi>(
         [](T l, T r) {
            return T(wrap_div(make_raw_arithmetic(l), make_raw_arithmetic(r)));
         },
         left, right
      );
   } else {
      return cat::simd<T, Abi>(left.raw / right.raw);
   }
}

template <typename T, typename Abi, overflow_policies policy>
   requires(is_integral<T> && !is_bool<T>)
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_mod_policy(
   cat::simd<T, Abi> const& left, cat::simd<T, Abi> const& right
) -> cat::simd<T, Abi> {
   if constexpr (
      (policy == overflow_policies::saturate
       || policy == overflow_policies::wrap)
      && is_signed<raw_arithmetic_type<T>>
   ) {
      using raw_scalar = raw_arithmetic_type<T>;
      return simd_integral_lanewise_op<T, Abi>(
         [](T l, T r) {
            auto const lr = make_raw_arithmetic(l);
            auto const rr = make_raw_arithmetic(r);
            if (rr == raw_scalar(-1) && lr == limits<raw_scalar>::min()) {
               return T(raw_scalar(0));
            }
            return T(lr % rr);
         },
         left, right
      );
   } else {
      return cat::simd<T, Abi>(left.raw % right.raw);
   }
}

template <typename T, typename Abi, overflow_policies policy>
   requires(is_integral<T> && !is_bool<T>)
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_shl_policy(
   cat::simd<T, Abi> const& left, cat::simd<T, Abi> const& right
) -> cat::simd<T, Abi> {
   if constexpr (
      policy == overflow_policies::saturate || policy == overflow_policies::wrap
   ) {
      return simd_integral_shift_lanes<T, Abi>(
         [](T l, T r) {
            return l << r;
         },
         left, right
      );
   } else {
      return cat::simd<T, Abi>(left.raw << right.raw);
   }
}

template <typename T, typename Abi, overflow_policies policy>
   requires(is_integral<T> && !is_bool<T>)
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_shr_policy(
   cat::simd<T, Abi> const& left, cat::simd<T, Abi> const& right
) -> cat::simd<T, Abi> {
   if constexpr (
      policy == overflow_policies::saturate || policy == overflow_policies::wrap
   ) {
      return simd_integral_shift_lanes<T, Abi>(
         [](T l, T r) {
            return l >> r;
         },
         left, right
      );
   } else {
      return cat::simd<T, Abi>(left.raw >> right.raw);
   }
}

template <typename T, typename Abi>
   requires(is_floating_point<T>)
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_floating_add_policy(
   cat::simd<T, Abi> const& left, cat::simd<T, Abi> const& right
) -> cat::simd<T, Abi> {
   // NOLINTBEGIN(bugprone-branch-clone)
   if constexpr (
      simd_float_precision_policy<T> == precision_policies::precise
   ) {
#pragma float_control(precise, on)
      return cat::simd<T, Abi>(left.raw + right.raw);
   } else {
#pragma float_control(precise, off)
      return cat::simd<T, Abi>(left.raw + right.raw);
   }
   // NOLINTEND(bugprone-branch-clone)
}

template <typename T, typename Abi>
   requires(is_floating_point<T>)
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_floating_sub_policy(
   cat::simd<T, Abi> const& left, cat::simd<T, Abi> const& right
) -> cat::simd<T, Abi> {
   // NOLINTBEGIN(bugprone-branch-clone)
   if constexpr (
      simd_float_precision_policy<T> == precision_policies::precise
   ) {
#pragma float_control(precise, on)
      return cat::simd<T, Abi>(left.raw - right.raw);
   } else {
#pragma float_control(precise, off)
      return cat::simd<T, Abi>(left.raw - right.raw);
   }
   // NOLINTEND(bugprone-branch-clone)
}

template <typename T, typename Abi>
   requires(is_floating_point<T>)
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_floating_mul_policy(
   cat::simd<T, Abi> const& left, cat::simd<T, Abi> const& right
) -> cat::simd<T, Abi> {
   // NOLINTBEGIN(bugprone-branch-clone)
   if constexpr (
      simd_float_precision_policy<T> == precision_policies::precise
   ) {
#pragma float_control(precise, on)
      return cat::simd<T, Abi>(left.raw * right.raw);
   } else {
#pragma float_control(precise, off)
      return cat::simd<T, Abi>(left.raw * right.raw);
   }
   // NOLINTEND(bugprone-branch-clone)
}

template <typename T, typename Abi>
   requires(is_floating_point<T>)
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_floating_div_policy(
   cat::simd<T, Abi> const& left, cat::simd<T, Abi> const& right
) -> cat::simd<T, Abi> {
   // NOLINTBEGIN(bugprone-branch-clone)
   if constexpr (
      simd_float_precision_policy<T> == precision_policies::precise
   ) {
#pragma float_control(precise, on)
      return cat::simd<T, Abi>(left.raw / right.raw);
   } else {
#pragma float_control(precise, off)
      return cat::simd<T, Abi>(left.raw / right.raw);
   }
   // NOLINTEND(bugprone-branch-clone)
}

template <typename T, typename Abi>
   requires(is_floating_point<T>)
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_floating_fma_policy(
   cat::simd<T, Abi> const& multiplicand, cat::simd<T, Abi> const& multiplier,
   cat::simd<T, Abi> const& addend
) -> cat::simd<T, Abi> {
   // Hardware FMA is a single-rounding op on both x86-64 (vfmadd...ps) and
   // NEON (vfmaq), so the surrounding precision policy does not change the
   // result.
   return cat::simd<T, Abi>(
      __builtin_elementwise_fma(multiplicand.raw, multiplier.raw, addend.raw)
   );
}

}  // namespace cat::detail
