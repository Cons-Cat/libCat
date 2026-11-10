// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/math>

namespace cat {
namespace detail {

// Conjugate the left-hand side of a Hermitian product. Real values pass
// through unchanged. A libCat complex uses `conj`, found by argument-dependent
// lookup once `<cat/complex>` is included. A raw `_Complex` is conjugated here.
template <typename T>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
hermitian_conjugate(T&& value) noexcept {
   using type = __typeof_unqual(value);
   if constexpr (is_raw_complex<type>) {
      type const number = value;
      using part = __typeof_unqual(__real number);
      if constexpr (is_floating_point<part>) {
         return __builtin_complex(__real number, part(-__imag number));
      } else {
         type result{};
         __real result = __real number;
         __imag result = part(-__imag number);
         return result;
      }
   } else if constexpr (is_complex<type>) {
      return conj(type(value));
   } else {
      return type(value);
   }
}

}  // namespace detail

// Bilinear product, `u * v`. Real arithmetic stays on this overload. A raw
// `_Complex` is arithmetic, but it is excluded here so the Hermitian overload
// is the one that matches.
template <typename T>
concept is_bilinear_dot_argument = is_arithmetic<T> && !is_complex<T>;

template <
   is_bilinear_dot_argument T, is_bilinear_dot_argument U,
   is_bilinear_dot_argument... Remaining>
   requires(sizeof...(Remaining) % 2 == 0)
[[nodiscard]]
constexpr auto
dot(T value_1, U value_2, Remaining... remaining) noexcept {
   if constexpr (sizeof...(Remaining) == 0) {
      return value_1 * value_2;
   } else {
      return value_1 * value_2 + dot(remaining...);
   }
}

// Hermitian inner product, math convention: `conj(u) * v`, summed over pairs.
// Conjugate-linear in each left element and linear in each right element.
template <typename T, typename U, typename... Remaining>
   requires(
      sizeof...(Remaining) % 2 == 0
      && (is_complex<T> || is_complex<U> || (is_complex<Remaining> || ...))
   )
[[nodiscard]]
constexpr auto
dot(T value_1, U value_2, Remaining... remaining) noexcept {
   if constexpr (sizeof...(Remaining) == 0) {
      return detail::hermitian_conjugate(value_1) * value_2;
   } else {
      return detail::hermitian_conjugate(value_1) * value_2 + dot(remaining...);
   }
}

}  // namespace cat
