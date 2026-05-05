// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/simd_arithmetic_policies.hpp>

namespace cat::detail {

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
      return simd_integral_add_policy<T, Abi, semantics>(*m_wrapped, rhs);
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
      return simd_integral_sub_policy<T, Abi, semantics>(*m_wrapped, rhs);
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
      return simd_integral_sub_policy<T, Abi, semantics>(lhs, *m_wrapped);
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
      return simd_integral_mul_policy<T, Abi, semantics>(*m_wrapped, rhs);
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
   divide_by(wrapper_type const& rhs) const -> wrapper_type {
      return simd_integral_div_policy<T, Abi, semantics>(*m_wrapped, rhs);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<remove_cvref<U>, T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_by(U&& rhs) const -> wrapper_type {
      return divide_by(wrapper_type($fwd(rhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_into(wrapper_type const& lhs) const -> wrapper_type {
      return simd_integral_div_policy<T, Abi, semantics>(lhs, *m_wrapped);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<remove_cvref<U>, T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_into(U&& lhs) const -> wrapper_type {
      return divide_into(wrapper_type($fwd(lhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   modulo_by(wrapper_type const& rhs) const -> wrapper_type {
      return simd_integral_mod_policy<T, Abi, semantics>(*m_wrapped, rhs);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<remove_cvref<U>, T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   modulo_by(U&& rhs) const -> wrapper_type {
      return modulo_by(wrapper_type($fwd(rhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   modulo_into(wrapper_type const& lhs) const -> wrapper_type {
      return simd_integral_mod_policy<T, Abi, semantics>(lhs, *m_wrapped);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<remove_cvref<U>, T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   modulo_into(U&& lhs) const -> wrapper_type {
      return modulo_into(wrapper_type($fwd(lhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_left_by(wrapper_type const& rhs) const -> wrapper_type {
      return simd_integral_shl_policy<T, Abi, semantics>(*m_wrapped, rhs);
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
      return simd_integral_shl_policy<T, Abi, semantics>(lhs, *m_wrapped);
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
      return simd_integral_shr_policy<T, Abi, semantics>(*m_wrapped, rhs);
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
      return simd_integral_shr_policy<T, Abi, semantics>(lhs, *m_wrapped);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<remove_cvref<U>, T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_right_into(U&& lhs) const -> wrapper_type {
      return shift_right_into(wrapper_type($fwd(lhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_and(wrapper_type const& rhs) const -> wrapper_type {
      return wrapper_type(m_wrapped->raw & rhs.raw);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<remove_cvref<U>, T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_and(U&& rhs) const -> wrapper_type {
      return bit_and(wrapper_type($fwd(rhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_or(wrapper_type const& rhs) const -> wrapper_type {
      return wrapper_type(m_wrapped->raw | rhs.raw);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<remove_cvref<U>, T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_or(U&& rhs) const -> wrapper_type {
      return bit_or(wrapper_type($fwd(rhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_xor(wrapper_type const& rhs) const -> wrapper_type {
      return wrapper_type(m_wrapped->raw ^ rhs.raw);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<remove_cvref<U>, T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_xor(U&& rhs) const -> wrapper_type {
      return bit_xor(wrapper_type($fwd(rhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_complement() const -> wrapper_type
      requires(is_unsigned_integral<raw_arithmetic_type<T>>)
   {
      return wrapper_type(~m_wrapped->raw);
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   fma(wrapper_type const& multiplier, wrapper_type const& addend) const
      -> wrapper_type {
      return simd_integral_add_policy<T, Abi, semantics>(
         simd_integral_mul_policy<T, Abi, semantics>(*m_wrapped, multiplier),
         addend);
   }
};

}  // namespace cat::detail
