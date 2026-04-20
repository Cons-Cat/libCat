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
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_add_follow_storage(cat::simd<T, Abi> const& left,
                                 cat::simd<T, Abi> const& right)
   -> cat::simd<T, Abi> {
   if constexpr (is_integral<T> && !is_bool<T>) {
      if constexpr (requires { T::policy; }) {
         if constexpr (T::policy == overflow_policies::saturate) {
            return cat::simd<T, Abi>(
               __builtin_elementwise_add_sat(left.raw, right.raw));
         }
      }
   }
   return cat::simd<T, Abi>(left.raw + right.raw);
}

template <typename T, typename Abi>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_sub_follow_storage(cat::simd<T, Abi> const& left,
                                 cat::simd<T, Abi> const& right)
   -> cat::simd<T, Abi> {
   if constexpr (is_integral<T> && !is_bool<T>) {
      if constexpr (requires { T::policy; }) {
         if constexpr (T::policy == overflow_policies::saturate) {
            return cat::simd<T, Abi>(
               __builtin_elementwise_sub_sat(left.raw, right.raw));
         }
      }
   }
   return cat::simd<T, Abi>(left.raw - right.raw);
}

template <typename T, typename Abi>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_mul_follow_storage(cat::simd<T, Abi> const& left,
                                 cat::simd<T, Abi> const& right)
   -> cat::simd<T, Abi> {
   if constexpr (is_integral<T> && !is_bool<T>) {
      if constexpr (requires { T::policy; }) {
         if constexpr (T::policy == overflow_policies::saturate) {
            return simd_integral_mul_sat_lanes(left, right);
         }
      }
   }
   return cat::simd<T, Abi>(left.raw * right.raw);
}

template <typename T, typename Abi>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_shl_follow_storage(cat::simd<T, Abi> const& left,
                                 cat::simd<T, Abi> const& right)
   -> cat::simd<T, Abi> {
   if constexpr (is_integral<T> && !is_bool<T>) {
      if constexpr (requires { T::policy; }) {
         if constexpr (T::policy == overflow_policies::saturate) {
            return simd_integral_shift_lanes<T, Abi>(
               [](T l, T r) {
                  return l << r;
               },
               left, right);
         }
      }
   }
   return cat::simd<T, Abi>(left.raw << right.raw);
}

template <typename T, typename Abi>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_shr_follow_storage(cat::simd<T, Abi> const& left,
                                 cat::simd<T, Abi> const& right)
   -> cat::simd<T, Abi> {
   if constexpr (is_integral<T> && !is_bool<T>) {
      if constexpr (requires { T::policy; }) {
         if constexpr (T::policy == overflow_policies::saturate) {
            return simd_integral_shift_lanes<T, Abi>(
               [](T l, T r) {
                  return l >> r;
               },
               left, right);
         }
      }
   }
   return cat::simd<T, Abi>(left.raw >> right.raw);
}

template <typename T, typename Abi, overflow_policies policy>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_add_forced(cat::simd<T, Abi> const& left,
                         cat::simd<T, Abi> const& right) -> cat::simd<T, Abi> {
   if constexpr (!is_integral<T> || is_bool<T>) {
      return cat::simd<T, Abi>(left.raw + right.raw);
   }
   if constexpr (policy == overflow_policies::saturate) {
      return cat::simd<T, Abi>(
         __builtin_elementwise_add_sat(left.raw, right.raw));
   }
   return cat::simd<T, Abi>(left.raw + right.raw);
}

template <typename T, typename Abi, overflow_policies policy>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_sub_forced(cat::simd<T, Abi> const& left,
                         cat::simd<T, Abi> const& right) -> cat::simd<T, Abi> {
   if constexpr (!is_integral<T> || is_bool<T>) {
      return cat::simd<T, Abi>(left.raw - right.raw);
   }
   if constexpr (policy == overflow_policies::saturate) {
      return cat::simd<T, Abi>(
         __builtin_elementwise_sub_sat(left.raw, right.raw));
   }
   return cat::simd<T, Abi>(left.raw - right.raw);
}

template <typename T, typename Abi, overflow_policies policy>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_mul_forced(cat::simd<T, Abi> const& left,
                         cat::simd<T, Abi> const& right) -> cat::simd<T, Abi> {
   if constexpr (!is_integral<T> || is_bool<T>) {
      return cat::simd<T, Abi>(left.raw * right.raw);
   }
   if constexpr (policy == overflow_policies::saturate) {
      return simd_integral_mul_sat_lanes(left, right);
   }
   return cat::simd<T, Abi>(left.raw * right.raw);
}

template <typename T, typename Abi, overflow_policies policy>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_shl_forced(cat::simd<T, Abi> const& left,
                         cat::simd<T, Abi> const& right) -> cat::simd<T, Abi> {
   if constexpr (!is_integral<T> || is_bool<T>) {
      return cat::simd<T, Abi>(left.raw << right.raw);
   }
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
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_integral_shr_forced(cat::simd<T, Abi> const& left,
                         cat::simd<T, Abi> const& right) -> cat::simd<T, Abi> {
   if constexpr (!is_integral<T> || is_bool<T>) {
      return cat::simd<T, Abi>(left.raw >> right.raw);
   }
   if constexpr (policy == overflow_policies::saturate) {
      return simd_integral_shift_lanes<T, Abi>(
         [](T l, T r) {
            return l >> r;
         },
         left, right);
   }
   return cat::simd<T, Abi>(left.raw >> right.raw);
}

template <typename T, typename Abi, overflow_policies Semantics>
class simd_overflow_reference {
   cat::simd<T, Abi>* m_wrapped;

 public:
   constexpr explicit simd_overflow_reference(cat::simd<T, Abi>& w)
       : m_wrapped(__builtin_addressof(w)) {
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator+(simd_overflow_reference lhs, cat::simd<T, Abi> const& rhs)
      -> cat::simd<T, Abi> {
      return simd_integral_add_forced<T, Abi, Semantics>(*lhs.m_wrapped, rhs);
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator+(cat::simd<T, Abi> const& lhs, simd_overflow_reference rhs)
      -> cat::simd<T, Abi> {
      return rhs + lhs;
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator-(simd_overflow_reference lhs, cat::simd<T, Abi> const& rhs)
      -> cat::simd<T, Abi> {
      return simd_integral_sub_forced<T, Abi, Semantics>(*lhs.m_wrapped, rhs);
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator-(cat::simd<T, Abi> const& lhs, simd_overflow_reference rhs)
      -> cat::simd<T, Abi> {
      return simd_integral_sub_forced<T, Abi, Semantics>(lhs, *rhs.m_wrapped);
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator*(simd_overflow_reference lhs, cat::simd<T, Abi> const& rhs)
      -> cat::simd<T, Abi> {
      return simd_integral_mul_forced<T, Abi, Semantics>(*lhs.m_wrapped, rhs);
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator*(cat::simd<T, Abi> const& lhs, simd_overflow_reference rhs)
      -> cat::simd<T, Abi> {
      return rhs * lhs;
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator<<(simd_overflow_reference lhs, cat::simd<T, Abi> const& rhs)
      -> cat::simd<T, Abi> {
      return simd_integral_shl_forced<T, Abi, Semantics>(*lhs.m_wrapped, rhs);
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator<<(cat::simd<T, Abi> const& lhs, simd_overflow_reference rhs)
      -> cat::simd<T, Abi> {
      return simd_integral_shl_forced<T, Abi, Semantics>(lhs, *rhs.m_wrapped);
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator>>(simd_overflow_reference lhs, cat::simd<T, Abi> const& rhs)
      -> cat::simd<T, Abi> {
      return simd_integral_shr_forced<T, Abi, Semantics>(*lhs.m_wrapped, rhs);
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator>>(cat::simd<T, Abi> const& lhs, simd_overflow_reference rhs)
      -> cat::simd<T, Abi> {
      return simd_integral_shr_forced<T, Abi, Semantics>(lhs, *rhs.m_wrapped);
   }
};

}  // namespace cat::detail
