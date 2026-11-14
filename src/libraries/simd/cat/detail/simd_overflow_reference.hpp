// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/simd_arithmetic_policies.hpp>

namespace cat {

template <typename T, typename CharT>
struct formatter;

}  // namespace cat

namespace cat::detail {

template <typename T, typename Abi, overflow_policies semantics>
class simd_overflow_reference
    : public arithmetic_interface<simd_overflow_reference<T, Abi, semantics>> {
 private:
   using wrapper_type = cat::simd<T, Abi>;

   wrapper_type* _Nonnull m_wrapped;

   template <typename, typename>
   friend struct ::cat::formatter;

 public:
   constexpr explicit simd_overflow_reference(
      wrapper_type& w [[clang::lifetimebound]]
   ) noexcept
       : m_wrapped(addressof(w)) {
   }

   // P3207R0. This type is reference-like. Friend deletion keeps binary
   // `operator&` from `arithmetic_interface` visible.
   friend constexpr void
   operator&(simd_overflow_reference const volatile&) = delete;

   friend constexpr void
   operator&(simd_overflow_reference const volatile&&) = delete;

   constexpr simd_overflow_reference(simd_overflow_reference const&) = default;

   // Assigning another reference of this type writes its value through, as
   // assigning a built-in reference does. `rebind()` changes the address.
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator=(simd_overflow_reference const& other) noexcept
      -> simd_overflow_reference& {
      *m_wrapped = *other.m_wrapped;
      return *this;
   }

   // Assign through to the referenced vector.
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator=(wrapper_type const& value) noexcept -> simd_overflow_reference& {
      *m_wrapped = value;
      return *this;
   }

   // A reference converts implicitly into its own value with this policy.
   [[gnu::always_inline, gnu::nodebug]]
   constexpr
   operator auto() const noexcept {
      return view();
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator-() const noexcept {
      return -view();
   }

   // Rebind this reference wrapper to a different address.
   constexpr void
   rebind(wrapper_type& w [[clang::lifetime_capture_by_this]]) noexcept {
      m_wrapped = addressof(w);
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   add(wrapper_type const& rhs) const noexcept -> wrapper_type {
      return simd_integral_add_policy<T, Abi, semantics>(*m_wrapped, rhs);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<__typeof_unqual(U), T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   add(U&& rhs) const -> wrapper_type {
      return add(wrapper_type($fwd(rhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_by(wrapper_type const& rhs) const noexcept -> wrapper_type {
      return simd_integral_sub_policy<T, Abi, semantics>(*m_wrapped, rhs);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<__typeof_unqual(U), T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_by(U&& rhs) const -> wrapper_type {
      return subtract_by(wrapper_type($fwd(rhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_from(wrapper_type const& lhs) const noexcept -> wrapper_type {
      return simd_integral_sub_policy<T, Abi, semantics>(lhs, *m_wrapped);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<__typeof_unqual(U), T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_from(U&& lhs) const -> wrapper_type {
      return subtract_from(wrapper_type($fwd(lhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   multiply(wrapper_type const& rhs) const noexcept -> wrapper_type {
      return simd_integral_mul_policy<T, Abi, semantics>(*m_wrapped, rhs);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<__typeof_unqual(U), T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   multiply(U&& rhs) const -> wrapper_type {
      return multiply(wrapper_type($fwd(rhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_by(wrapper_type const& rhs) const noexcept -> wrapper_type {
      return simd_integral_div_policy<T, Abi, semantics>(*m_wrapped, rhs);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<__typeof_unqual(U), T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_by(U&& rhs) const -> wrapper_type {
      return divide_by(wrapper_type($fwd(rhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_into(wrapper_type const& lhs) const noexcept -> wrapper_type {
      return simd_integral_div_policy<T, Abi, semantics>(lhs, *m_wrapped);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<__typeof_unqual(U), T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_into(U&& lhs) const -> wrapper_type {
      return divide_into(wrapper_type($fwd(lhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   modulo_by(wrapper_type const& rhs) const noexcept -> wrapper_type {
      return simd_integral_mod_policy<T, Abi, semantics>(*m_wrapped, rhs);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<__typeof_unqual(U), T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   modulo_by(U&& rhs) const -> wrapper_type {
      return modulo_by(wrapper_type($fwd(rhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   modulo_into(wrapper_type const& lhs) const noexcept -> wrapper_type {
      return simd_integral_mod_policy<T, Abi, semantics>(lhs, *m_wrapped);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<__typeof_unqual(U), T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   modulo_into(U&& lhs) const -> wrapper_type {
      return modulo_into(wrapper_type($fwd(lhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_left_by(wrapper_type const& rhs) const noexcept -> wrapper_type {
      return simd_integral_shl_policy<T, Abi, semantics>(*m_wrapped, rhs);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<__typeof_unqual(U), T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_left_by(U&& rhs) const -> wrapper_type {
      return shift_left_by(wrapper_type($fwd(rhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_left_into(wrapper_type const& lhs) const noexcept -> wrapper_type {
      return simd_integral_shl_policy<T, Abi, semantics>(lhs, *m_wrapped);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<__typeof_unqual(U), T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_left_into(U&& lhs) const -> wrapper_type {
      return shift_left_into(wrapper_type($fwd(lhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_right_by(wrapper_type const& rhs) const noexcept -> wrapper_type {
      return simd_integral_shr_policy<T, Abi, semantics>(*m_wrapped, rhs);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<__typeof_unqual(U), T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_right_by(U&& rhs) const -> wrapper_type {
      return shift_right_by(wrapper_type($fwd(rhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_right_into(wrapper_type const& lhs) const noexcept -> wrapper_type {
      return simd_integral_shr_policy<T, Abi, semantics>(lhs, *m_wrapped);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<__typeof_unqual(U), T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_right_into(U&& lhs) const -> wrapper_type {
      return shift_right_into(wrapper_type($fwd(lhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_and(wrapper_type const& rhs) const noexcept -> wrapper_type {
      return wrapper_type(m_wrapped->raw & rhs.raw);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<__typeof_unqual(U), T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_and(U&& rhs) const -> wrapper_type {
      return bit_and(wrapper_type($fwd(rhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_or(wrapper_type const& rhs) const noexcept -> wrapper_type {
      return wrapper_type(m_wrapped->raw | rhs.raw);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<__typeof_unqual(U), T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_or(U&& rhs) const -> wrapper_type {
      return bit_or(wrapper_type($fwd(rhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_xor(wrapper_type const& rhs) const noexcept -> wrapper_type {
      return wrapper_type(m_wrapped->raw ^ rhs.raw);
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<__typeof_unqual(U), T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_xor(U&& rhs) const -> wrapper_type {
      return bit_xor(wrapper_type($fwd(rhs)));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_complement() const noexcept -> wrapper_type
      requires(is_unsigned_integral<raw_arithmetic_type<T>>)
   {
      return wrapper_type(~m_wrapped->raw);
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   fma(
      wrapper_type const& multiplier, wrapper_type const& addend
   ) const noexcept -> wrapper_type {
      return simd_integral_add_policy<T, Abi, semantics>(
         simd_integral_mul_policy<T, Abi, semantics>(*m_wrapped, multiplier),
         addend
      );
   }

 private:
   // The referenced vector, copied bit for bit into lanes with this policy.
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   view() const noexcept {
      using value = cat::simd<
         typename rebind_overflow_policy<T, semantics>::type,
         typename Abi::template make_abi_type<
            typename rebind_overflow_policy<T, semantics>::type>>;
      return value(
         __builtin_bit_cast(typename value::raw_type, m_wrapped->raw)
      );
   }
};

}  // namespace cat::detail

namespace cat {

// Implementing this here is a circular dependency. The implementation can be
// found in <cat/simd/implementations/format_simd_overflow_reference.tpp>.
template <typename T, typename Abi, overflow_policies policy, typename CharT>
struct formatter<detail::simd_overflow_reference<T, Abi, policy>, CharT>;

}  // namespace cat
