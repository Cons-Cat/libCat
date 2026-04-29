// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/bit>
#include <cat/maybe>

namespace cat {

// Count the number of leading one bits (counting "leftwards") before the first
// found zero bit, wrapped in a `cat::maybe`. When the input is zero, this
// returns `nullopt`.
template <is_unsigned_integral T>
   requires(!detail::is_idx<T>)
[[nodiscard]]
constexpr auto
countl_zero_raw(T value) -> maybe<uint1> {
   if (value == 0u) {
      return nullopt;
   }
   // This intrinsic returns `int` but it can't exceed 64 so it's safe to cast.
   return uint1(__builtin_clzg(make_raw_arithmetic(value)));
}

// Count the number of leading one bits (counting "leftwards") before the first
// found zero bit, wrapped in a `cat::maybe`. When the input is zero, this
// returns `nullopt`.
template <is_unsigned_integral T>
   requires(!detail::is_idx<T>)
[[nodiscard]]
constexpr auto
countl_one_raw(T value) -> maybe<uint1> {
   if (value == 0u) {
      return nullopt;
   }
   // This intrinsic returns `int` but it can't exceed 64 so it's safe to cast.
   return uint1(__builtin_clzg(
      // ~ casts raw integers to `int`, so we need to cast back.
      raw_arithmetic_type<T>(~make_raw_arithmetic(value))));
}

// Count the number of trailing zero bits (counting "rightwards") before the
// first found one bit, wrapped in a `cat::maybe`. When the input is zero, this
// returns `nullopt`.
template <is_unsigned_integral T>
[[nodiscard]]
constexpr auto
countr_zero_raw(T value) -> maybe<uint1> {
   if (value == 0u) {
      return nullopt;
   }
   // This intrinsic returns `int` but it can't exceed 64 so it's safe to cast.
   return uint1(__builtin_ctzg(make_raw_arithmetic(value)));
}

// Count the number of trailing one bits (counting "rightwards") before the
// first found zero bit, wrapped in a `cat::maybe`. When the input is zero, this
// returns `nullopt`.
template <is_unsigned_integral T>
[[nodiscard]]
constexpr auto
countr_one_raw(T value) -> maybe<uint1> {
   if (value == 0u) {
      return nullopt;
   }
   // This intrinsic returns `int` but it can't exceed 64 so it's safe to cast.
   return uint1(__builtin_ctzg(
      // ~ casts raw integers to `int`, so we need to cast back.
      raw_arithmetic_type<T>(~make_raw_arithmetic(value))));
}

template <is_unsigned_integral T>
[[nodiscard]]
constexpr auto
bit_ceil_raw(T value) -> maybe<T> {
   if (make_raw_arithmetic(value)
       > raw_arithmetic_type<T>(limits<T>::high_bit)) {
      return nullopt;
   }
   return T(__builtin_stdc_bit_ceil(make_raw_arithmetic(value)));
}

}  // namespace cat
