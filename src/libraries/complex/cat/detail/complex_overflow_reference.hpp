// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/complex_reference.hpp>

// Included by <cat/complex> after `basic_complex_int` is defined.

namespace cat {

// This proxy reference reads and writes a `cat::basic_complex_int` with a
// specific overflow policy. It is obtained from `.undef()`, `.wrap()`, and
// `.sat()`.
template <typename Complex, overflow_policies overflow_policy>
class complex_overflow_reference
    : public detail::complex_reference<
         complex_overflow_reference<Complex, overflow_policy>, Complex,
         basic_complex_int<
            typename remove_cv<Complex>::component_type, overflow_policy,
            remove_cv<Complex>::quantity_reference>>,
      public plus_interface<
         complex_overflow_reference<Complex, overflow_policy>>,
      public minus_interface<
         complex_overflow_reference<Complex, overflow_policy>>,
      public multiply_interface<
         complex_overflow_reference<Complex, overflow_policy>>,
      public divide_interface<
         complex_overflow_reference<Complex, overflow_policy>> {
   using base = detail::complex_reference<
      complex_overflow_reference, Complex,
      basic_complex_int<
         typename remove_cv<Complex>::component_type, overflow_policy,
         remove_cv<Complex>::quantity_reference>>;

   template <overflow_policies other_policy>
   using value_with = basic_complex_int<
      typename remove_cv<Complex>::component_type, other_policy,
      remove_cv<Complex>::quantity_reference>;

   template <overflow_policies other_policy>
   using rebound = complex_overflow_reference<Complex, other_policy>;

   template <overflow_policies other_policy>
   using rebound_const =
      complex_overflow_reference<Complex const, other_policy>;

 public:
   using base::base;
   using base::operator=;

   static constexpr overflow_policies policy = overflow_policy;

   constexpr auto
   undef() & noexcept -> rebound<overflow_policies::undefined> {
      return rebound<overflow_policies::undefined>(*this->m_wrapped);
   }

   constexpr auto
   undef() const& noexcept -> rebound_const<overflow_policies::undefined> {
      return rebound_const<overflow_policies::undefined>(*this->m_wrapped);
   }

   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   undef() const&& noexcept {
      return value_with<overflow_policies::undefined>(
         detail::complex_in_raw<typename base::component_type>(*this->m_wrapped)
      );
   }

   constexpr auto
   wrap() & noexcept -> rebound<overflow_policies::wrap> {
      return rebound<overflow_policies::wrap>(*this->m_wrapped);
   }

   constexpr auto
   wrap() const& noexcept -> rebound_const<overflow_policies::wrap> {
      return rebound_const<overflow_policies::wrap>(*this->m_wrapped);
   }

   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   wrap() const&& noexcept {
      return value_with<overflow_policies::wrap>(
         detail::complex_in_raw<typename base::component_type>(*this->m_wrapped)
      );
   }

   constexpr auto
   sat() & noexcept -> rebound<overflow_policies::saturate> {
      return rebound<overflow_policies::saturate>(*this->m_wrapped);
   }

   constexpr auto
   sat() const& noexcept -> rebound_const<overflow_policies::saturate> {
      return rebound_const<overflow_policies::saturate>(*this->m_wrapped);
   }

   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   sat() const&& noexcept {
      return value_with<overflow_policies::saturate>(
         detail::complex_in_raw<typename base::component_type>(*this->m_wrapped)
      );
   }
};

namespace detail {
template <typename Complex, overflow_policies policy>
struct complex_reference_view_trait<
   complex_overflow_reference<Complex, policy>> {
   using type = basic_complex_int<
      typename remove_cv<Complex>::component_type, policy,
      remove_cv<Complex>::quantity_reference>;
};
}  // namespace detail

namespace detail {
template <typename Complex, overflow_policies policy>
inline constexpr bool
   is_complex_reference<complex_overflow_reference<Complex, policy>> = true;
}  // namespace detail

}  // namespace cat
