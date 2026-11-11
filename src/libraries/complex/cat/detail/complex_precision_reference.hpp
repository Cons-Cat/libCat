// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/complex_reference.hpp>

// Included by <cat/complex> after `basic_complex_float` is defined.

namespace cat {

// This proxy reference reads and writes a `cat::basic_complex_float` with a
// specific precision policy. It is obtained from `.precise()` and `.fast()`.
template <typename Complex, precision_policies precision>
class complex_precision_reference
    : public detail::complex_reference<
         complex_precision_reference<Complex, precision>, Complex,
         basic_complex_float<
            typename remove_cv<Complex>::component_type, precision,
            remove_cv<Complex>::quantity_reference>>,
      public plus_interface<complex_precision_reference<Complex, precision>>,
      public minus_interface<complex_precision_reference<Complex, precision>>,
      public multiply_interface<
         complex_precision_reference<Complex, precision>>,
      public divide_interface<complex_precision_reference<Complex, precision>> {
   using base = detail::complex_reference<
      complex_precision_reference, Complex,
      basic_complex_float<
         typename remove_cv<Complex>::component_type, precision,
         remove_cv<Complex>::quantity_reference>>;

   template <precision_policies other_precision>
   using value_with = basic_complex_float<
      typename remove_cv<Complex>::component_type, other_precision,
      remove_cv<Complex>::quantity_reference>;

   template <precision_policies other_precision>
   using rebound = complex_precision_reference<Complex, other_precision>;

   template <precision_policies other_precision>
   using rebound_const =
      complex_precision_reference<Complex const, other_precision>;

 public:
   using base::base;
   using base::operator=;

   static constexpr precision_policies policy = precision;

   constexpr auto
   precise() & noexcept -> rebound<precision_policies::precise> {
      return rebound<precision_policies::precise>(*this->m_wrapped);
   }

   constexpr auto
   precise() const& noexcept -> rebound_const<precision_policies::precise> {
      return rebound_const<precision_policies::precise>(*this->m_wrapped);
   }

   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   precise() const&& noexcept {
      return value_with<precision_policies::precise>(
         detail::complex_in_raw<typename base::component_type>(*this->m_wrapped)
      );
   }

   constexpr auto
   fast() & noexcept -> rebound<precision_policies::fast> {
      return rebound<precision_policies::fast>(*this->m_wrapped);
   }

   constexpr auto
   fast() const& noexcept -> rebound_const<precision_policies::fast> {
      return rebound_const<precision_policies::fast>(*this->m_wrapped);
   }

   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   fast() const&& noexcept {
      return value_with<precision_policies::fast>(
         detail::complex_in_raw<typename base::component_type>(*this->m_wrapped)
      );
   }
};

namespace detail {
template <typename Complex, precision_policies precision>
struct complex_reference_view_trait<
   complex_precision_reference<Complex, precision>> {
   using type = basic_complex_float<
      typename remove_cv<Complex>::component_type, precision,
      remove_cv<Complex>::quantity_reference>;
};
}  // namespace detail

namespace detail {
template <typename Complex, precision_policies precision>
inline constexpr bool
   is_complex_reference<complex_precision_reference<Complex, precision>> = true;
}  // namespace detail

}  // namespace cat
