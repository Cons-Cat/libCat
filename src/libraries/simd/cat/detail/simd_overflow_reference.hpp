// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>

namespace cat::detail {

template <typename T, typename Abi>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_mul_sat_lanes(cat::simd<T, Abi> const& left,
                            cat::simd<T, Abi> const& right)
   -> cat::simd<T, Abi> {
   cat::simd<T, Abi> out{};
   for (idx i = 0u; i < left.size(); ++i) {
      out.set_lane(i, left[i] * right[i]);
   }
   return out;
}

template <typename T, typename Abi, typename F>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_shift_lanes(F&& shift_fn, cat::simd<T, Abi> const& left,
                          cat::simd<T, Abi> const& right) -> cat::simd<T, Abi> {
   cat::simd<T, Abi> out{};
   for (idx i = 0u; i < left.size(); ++i) {
      out.set_lane(i, shift_fn(left[i], right[i]));
   }
   return out;
}

template <typename T, typename Abi>
   requires(is_integral<T> && !is_bool<T>)
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_add_follow_storage(cat::simd<T, Abi> const& left,
                                 cat::simd<T, Abi> const& right)
   -> cat::simd<T, Abi> {
   if constexpr (requires { T::policy; }) {
      if constexpr (T::policy == overflow_policies::saturate) {
         return cat::simd<T, Abi>(
            __builtin_elementwise_add_sat(left.raw, right.raw));
      }
   }
   return cat::simd<T, Abi>(left.raw + right.raw);
}

template <typename T, typename Abi>
   requires(is_integral<T> && !is_bool<T>)
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_sub_follow_storage(cat::simd<T, Abi> const& left,
                                 cat::simd<T, Abi> const& right)
   -> cat::simd<T, Abi> {
   if constexpr (requires { T::policy; }) {
      if constexpr (T::policy == overflow_policies::saturate) {
         return cat::simd<T, Abi>(
            __builtin_elementwise_sub_sat(left.raw, right.raw));
      }
   }
   return cat::simd<T, Abi>(left.raw - right.raw);
}

template <typename T, typename Abi>
   requires(is_integral<T> && !is_bool<T>)
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_mul_follow_storage(cat::simd<T, Abi> const& left,
                                 cat::simd<T, Abi> const& right)
   -> cat::simd<T, Abi> {
   if constexpr (requires { T::policy; }) {
      if constexpr (T::policy == overflow_policies::saturate) {
         return simd_integral_mul_sat_lanes(left, right);
      }
   }
   return cat::simd<T, Abi>(left.raw * right.raw);
}

template <typename T, typename Abi>
   requires(is_integral<T> && !is_bool<T>)
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_shl_follow_storage(cat::simd<T, Abi> const& left,
                                 cat::simd<T, Abi> const& right)
   -> cat::simd<T, Abi> {
   if constexpr (requires { T::policy; }) {
      if constexpr (T::policy == overflow_policies::saturate) {
         return simd_integral_shift_lanes<T, Abi>(
            [](T l, T r) {
               return l << r;
            },
            left, right);
      }
   }
   return cat::simd<T, Abi>(left.raw << right.raw);
}

template <typename T, typename Abi>
   requires(is_integral<T> && !is_bool<T>)
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_shr_follow_storage(cat::simd<T, Abi> const& left,
                                 cat::simd<T, Abi> const& right)
   -> cat::simd<T, Abi> {
   if constexpr (requires { T::policy; }) {
      if constexpr (T::policy == overflow_policies::saturate) {
         return simd_integral_shift_lanes<T, Abi>(
            [](T l, T r) {
               return l >> r;
            },
            left, right);
      }
   }
   return cat::simd<T, Abi>(left.raw >> right.raw);
}

template <typename T, typename Abi, overflow_policies policy>
   requires(is_integral<T> && !is_bool<T>)
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_add_forced(cat::simd<T, Abi> const& left,
                         cat::simd<T, Abi> const& right) -> cat::simd<T, Abi> {
   if constexpr (policy == overflow_policies::saturate) {
      return cat::simd<T, Abi>(
         __builtin_elementwise_add_sat(left.raw, right.raw));
   }
   return cat::simd<T, Abi>(left.raw + right.raw);
}

template <typename T, typename Abi, overflow_policies policy>
   requires(is_integral<T> && !is_bool<T>)
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_sub_forced(cat::simd<T, Abi> const& left,
                         cat::simd<T, Abi> const& right) -> cat::simd<T, Abi> {
   if constexpr (policy == overflow_policies::saturate) {
      return cat::simd<T, Abi>(
         __builtin_elementwise_sub_sat(left.raw, right.raw));
   }
   return cat::simd<T, Abi>(left.raw - right.raw);
}

template <typename T, typename Abi, overflow_policies policy>
   requires(is_integral<T> && !is_bool<T>)
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_mul_forced(cat::simd<T, Abi> const& left,
                         cat::simd<T, Abi> const& right) -> cat::simd<T, Abi> {
   if constexpr (policy == overflow_policies::saturate) {
      return simd_integral_mul_sat_lanes(left, right);
   }
   return cat::simd<T, Abi>(left.raw * right.raw);
}

template <typename T, typename Abi, overflow_policies policy>
   requires(is_integral<T> && !is_bool<T>)
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_shl_forced(cat::simd<T, Abi> const& left,
                         cat::simd<T, Abi> const& right) -> cat::simd<T, Abi> {
   if constexpr (policy == overflow_policies::saturate) {
      return simd_integral_shift_lanes<T, Abi>(
         [](T l, T r) {
            return l << r;
         },
         left, right);
   }
   return cat::simd<T, Abi>(left.raw << right.raw);
}

template <typename T, typename Abi, overflow_policies policy>
   requires(is_integral<T> && !is_bool<T>)
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_shr_forced(cat::simd<T, Abi> const& left,
                         cat::simd<T, Abi> const& right) -> cat::simd<T, Abi> {
   if constexpr (policy == overflow_policies::saturate) {
      return simd_integral_shift_lanes<T, Abi>(
         [](T l, T r) {
            return l >> r;
         },
         left, right);
   }
   return cat::simd<T, Abi>(left.raw >> right.raw);
}

template <typename T, typename Abi, overflow_policies semantics>
class simd_overflow_reference
    : public arithmetic_interface<simd_overflow_reference<T, Abi, semantics>> {
   using wrapper_type = cat::simd<T, Abi>;

   wrapper_type* m_wrapped;

 public:
   constexpr explicit simd_overflow_reference(wrapper_type& w)
       : m_wrapped(__builtin_addressof(w)) {
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   add(wrapper_type const& rhs) const -> wrapper_type {
      return simd_integral_add_forced<T, Abi, semantics>(*m_wrapped, rhs);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<remove_cvref<U>, T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   add(U&& rhs) const -> wrapper_type {
      return add(wrapper_type($fwd(rhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_by(wrapper_type const& rhs) const -> wrapper_type {
      return simd_integral_sub_forced<T, Abi, semantics>(*m_wrapped, rhs);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<remove_cvref<U>, T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_by(U&& rhs) const -> wrapper_type {
      return subtract_by(wrapper_type($fwd(rhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_from(wrapper_type const& lhs) const -> wrapper_type {
      return simd_integral_sub_forced<T, Abi, semantics>(lhs, *m_wrapped);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<remove_cvref<U>, T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_from(U&& lhs) const -> wrapper_type {
      return subtract_from(wrapper_type($fwd(lhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   multiply(wrapper_type const& rhs) const -> wrapper_type {
      return simd_integral_mul_forced<T, Abi, semantics>(*m_wrapped, rhs);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<remove_cvref<U>, T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   multiply(U&& rhs) const -> wrapper_type {
      return multiply(wrapper_type($fwd(rhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_left_by(wrapper_type const& rhs) const -> wrapper_type {
      return simd_integral_shl_forced<T, Abi, semantics>(*m_wrapped, rhs);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<remove_cvref<U>, T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_left_by(U&& rhs) const -> wrapper_type {
      return shift_left_by(wrapper_type($fwd(rhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_left_into(wrapper_type const& lhs) const -> wrapper_type {
      return simd_integral_shl_forced<T, Abi, semantics>(lhs, *m_wrapped);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<remove_cvref<U>, T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_left_into(U&& lhs) const -> wrapper_type {
      return shift_left_into(wrapper_type($fwd(lhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_right_by(wrapper_type const& rhs) const -> wrapper_type {
      return simd_integral_shr_forced<T, Abi, semantics>(*m_wrapped, rhs);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<remove_cvref<U>, T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_right_by(U&& rhs) const -> wrapper_type {
      return shift_right_by(wrapper_type($fwd(rhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_right_into(wrapper_type const& lhs) const -> wrapper_type {
      return simd_integral_shr_forced<T, Abi, semantics>(lhs, *m_wrapped);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<remove_cvref<U>, T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_right_into(U&& lhs) const -> wrapper_type {
      return shift_right_into(wrapper_type($fwd(lhs)));
   }
};

}  // namespace cat::detail
