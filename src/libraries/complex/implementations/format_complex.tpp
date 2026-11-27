// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/complex>
#include <cat/format>
#include <cat/math>

// `<cat/complex>` is on `<cat/format>`'s include graph once this file is
// pulled in, so the specializations cannot be defined in the complex header.
// They are declared there and defined here after `formatter_base` exists.

namespace cat {

namespace detail {

template <typename Part, typename CharT, is_allocator Allocator>
auto
format_complex_part(
   allocator_ref<Allocator> allocator, Part const& part, format_context context
) -> scaredy_format<void> {
   using raw_type = raw_arithmetic_type<Part>;
   return formatter<raw_type, CharT>{}.format(
      allocator, make_raw_arithmetic(part), context
   );
}

// `real + imag i`, with one unit suffix when the parts carry a quantity.
// The digits are the same ones `basic_int` and `basic_float` print. Those
// formatters do not print an overflow or precision policy, so neither does
// this.
template <typename Complex, typename CharT, is_allocator Allocator>
auto
format_complex(
   allocator_ref<Allocator> allocator, Complex const& value,
   format_context context
) -> scaredy_format<void> {
   using part = typename Complex::value_type;
   part const zero{};
   part const imag = value.imag();
   bool const negative = imag < zero;

   scaredy_format<void> result =
      format_complex_part<part, CharT>(allocator, value.real(), context);
   if (result.is_empty()) {
      return result;
   }

   result = context.append(negative ? str_view(" - ") : str_view(" + "));
   if (result.is_empty()) {
      return result;
   }

   if constexpr (is_integral<part>) {
      // The most negative integer's magnitude only fits unsigned.
      using raw_type = raw_arithmetic_type<part>;
      using magnitude_type = make_unsigned_type<raw_type>;
      magnitude_type const stored = make_unsigned(make_raw_arithmetic(imag));
      result = formatter<magnitude_type, CharT>{}.format(
         allocator,
         negative ? static_cast<magnitude_type>(magnitude_type(0) - stored)
                  : stored,
         context
      );
   } else {
      result = format_complex_part<part, CharT>(
         allocator, negative ? abs(imag) : imag, context
      );
   }

   if (result.is_empty()) {
      return result;
   }

   result = context.append('i');
   if (result.is_empty()) {
      return result;
   }

   if constexpr (has_quantity<part>) {
      return append_arithmetic_quantity_symbol<part>(context);
   }

   return monostate;
}

}  // namespace detail

template <typename T, overflow_policies policy, auto quantity, typename CharT>
struct formatter<basic_complex_int<T, policy, quantity>, CharT>
    : formatter_base<CharT> {
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> allocator,
      basic_complex_int<T, policy, quantity> const& value,
      format_context context
   ) const -> scaredy_format<void> {
      return detail::format_complex<
         basic_complex_int<T, policy, quantity>, CharT>(
         allocator, value, context
      );
   }

   auto
   format(
      allocator_dyn allocator,
      basic_complex_int<T, policy, quantity> const& value,
      format_context context
   ) const -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

template <
   typename T, precision_policies precision, auto quantity, typename CharT>
struct formatter<basic_complex_float<T, precision, quantity>, CharT>
    : formatter_base<CharT> {
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> allocator,
      basic_complex_float<T, precision, quantity> const& value,
      format_context context
   ) const -> scaredy_format<void> {
      return detail::format_complex<
         basic_complex_float<T, precision, quantity>, CharT>(
         allocator, value, context
      );
   }

   auto
   format(
      allocator_dyn allocator,
      basic_complex_float<T, precision, quantity> const& value,
      format_context context
   ) const -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

// A proxy reference formats as its value.
template <typename Complex, precision_policies precision, typename CharT>
struct formatter<complex_precision_reference<Complex, precision>, CharT>
    : formatter_base<CharT> {
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> allocator,
      complex_precision_reference<Complex, precision> const& value,
      format_context context
   ) const -> scaredy_format<void> {
      using view = detail::complex_reference_view<__typeof_unqual(value)>;
      view const temporary = value;
      return detail::format_complex<view, CharT>(allocator, temporary, context);
   }

   auto
   format(
      allocator_dyn allocator,
      complex_precision_reference<Complex, precision> const& value,
      format_context context
   ) const -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

template <typename Complex, overflow_policies overflow_policy, typename CharT>
struct formatter<complex_overflow_reference<Complex, overflow_policy>, CharT>
    : formatter_base<CharT> {
   template <is_allocator Allocator>
   auto
   format(
      allocator_ref<Allocator> allocator,
      complex_overflow_reference<Complex, overflow_policy> const& value,
      format_context context
   ) const -> scaredy_format<void> {
      using view = detail::complex_reference_view<__typeof_unqual(value)>;
      view const temporary = value;
      return detail::format_complex<view, CharT>(allocator, temporary, context);
   }

   auto
   format(
      allocator_dyn allocator,
      complex_overflow_reference<Complex, overflow_policy> const& value,
      format_context context
   ) const -> scaredy_format<void> {
      return format<allocator_dyn>(allocator, value, context);
   }
};

}  // namespace cat
