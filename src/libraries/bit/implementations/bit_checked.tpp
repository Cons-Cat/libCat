// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/bit>
#include <cat/maybe>

namespace cat {

template <is_unsigned_integral T>
   requires(!is_idx<T>)
[[nodiscard]]
constexpr auto
countl_zero_unchecked(T value) -> uint1 {
   return uint1(__builtin_clzg(make_raw_arithmetic(value)));
}

template <is_unsigned_integral T>
   requires(!is_idx<T>)
[[nodiscard]]
constexpr auto
countl_one_unchecked(T value) -> uint1 {
   return uint1(
      __builtin_clzg(raw_arithmetic_type<T>(~make_raw_arithmetic(value)))
   );
}

template <is_unsigned_integral T>
[[nodiscard]]
constexpr auto
countr_zero_unchecked(T value) -> uint1 {
   return uint1(__builtin_ctzg(make_raw_arithmetic(value)));
}

template <is_unsigned_integral T>
[[nodiscard]]
constexpr auto
countr_one_unchecked(T value) -> uint1 {
   return uint1(
      __builtin_ctzg(raw_arithmetic_type<T>(~make_raw_arithmetic(value)))
   );
}

template <is_unsigned_integral T>
[[nodiscard]]
constexpr auto
bit_ceil_unchecked(T value) -> T {
   return T(__builtin_stdc_bit_ceil(make_raw_arithmetic(value)));
}

template <is_unsigned_integral T>
   requires(!is_idx<T>)
[[nodiscard]]
constexpr auto
try_countl_zero(T value) -> maybe<uint1> {
   if (value == 0u) {
      return nullopt;
   }
   return countl_zero_unchecked(value);
}

template <is_unsigned_integral T>
   requires(!is_idx<T>)
[[nodiscard]]
constexpr auto
try_countl_one(T value) -> maybe<uint1> {
   if (make_raw_arithmetic(value) == limits<raw_arithmetic_type<T>>::max()) {
      return nullopt;
   }
   return countl_one_unchecked(value);
}

template <is_unsigned_integral T>
[[nodiscard]]
constexpr auto
try_countr_zero(T value) -> maybe<uint1> {
   if (value == 0u) {
      return nullopt;
   }
   return countr_zero_unchecked(value);
}

template <is_unsigned_integral T>
[[nodiscard]]
constexpr auto
try_countr_one(T value) -> maybe<uint1> {
   if (make_raw_arithmetic(value) == limits<raw_arithmetic_type<T>>::max()) {
      return nullopt;
   }
   return countr_one_unchecked(value);
}

template <is_unsigned_integral T>
[[nodiscard]]
constexpr auto
try_bit_ceil(T value) -> maybe<T> {
   if (value > raw_arithmetic_type<T>(limits<T>::high_bit)) {
      return nullopt;
   }
   return bit_ceil_unchecked(value);
}

[[nodiscard]]
constexpr auto
countl_zero_unchecked(byte value) -> uint1 {
   return countl_zero_unchecked(value.value);
}

[[nodiscard]]
constexpr auto
countl_one_unchecked(byte value) -> uint1 {
   return countl_one_unchecked(value.value);
}

[[nodiscard]]
constexpr auto
countr_zero_unchecked(byte value) -> uint1 {
   return countr_zero_unchecked(value.value);
}

[[nodiscard]]
constexpr auto
countr_one_unchecked(byte value) -> uint1 {
   return countr_one_unchecked(value.value);
}

[[nodiscard]]
constexpr auto
bit_ceil_unchecked(byte value) -> byte {
   return byte(bit_ceil_unchecked(value.value));
}

[[nodiscard]]
constexpr auto
try_countl_zero(byte value) -> maybe<uint1> {
   return try_countl_zero(value.value);
}

[[nodiscard]]
constexpr auto
try_countl_one(byte value) -> maybe<uint1> {
   return try_countl_one(value.value);
}

[[nodiscard]]
constexpr auto
try_countr_zero(byte value) -> maybe<uint1> {
   return try_countr_zero(value.value);
}

[[nodiscard]]
constexpr auto
try_countr_one(byte value) -> maybe<uint1> {
   return try_countr_one(value.value);
}

[[nodiscard]]
constexpr auto
try_bit_ceil(byte value) -> maybe<byte> {
   unsigned char const raw_value = make_raw_arithmetic(value.value);
   if (raw_value > static_cast<unsigned char>(1u << 7u)) {
      return nullopt;
   }
   return byte(__builtin_stdc_bit_ceil(raw_value));
}

}  // namespace cat
