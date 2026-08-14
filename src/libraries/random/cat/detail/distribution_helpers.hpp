// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/meta>
#include <cat/random>

namespace cat::detail {

template <uniform_random_bit_generator Generator>
constexpr auto
distribution_engine_word(Generator& generator) {
   using engine_type = remove_cvref<Generator>;
   using raw_type = raw_arithmetic_type<typename engine_type::result_type>;
   using unsigned_type = make_unsigned_type<raw_type>;
   unsigned_type const minimum =
      static_cast<unsigned_type>(make_raw_arithmetic(engine_type::min()));
   return static_cast<unsigned_type>(make_raw_arithmetic(generator()))
          - minimum;
}

template <uniform_random_bit_generator Generator>
constexpr auto
distribution_random_bit(Generator& generator) -> bool {
   using engine_type = remove_cvref<Generator>;
   using raw_type = raw_arithmetic_type<typename engine_type::result_type>;
   using unsigned_type = make_unsigned_type<raw_type>;
   unsigned_type const maximum =
      static_cast<unsigned_type>(make_raw_arithmetic(engine_type::max()));
   unsigned_type const minimum =
      static_cast<unsigned_type>(make_raw_arithmetic(engine_type::min()));
   unsigned_type const span = maximum - minimum + 1u;
   if (span == 0u) {
      return (distribution_engine_word(generator) & 1u) != 0u;
   }
   unsigned_type const limit = span - span % 2u;
   while (true) {
      unsigned_type const value = distribution_engine_word(generator);
      if (value < limit) {
         return (value & 1u) != 0u;
      }
   }
}

template <uniform_random_bit_generator Generator, is_raw_integral T>
   requires is_unsigned<T>
constexpr auto
distribution_random_bounded(Generator& generator, T bound) -> T {
   using engine_type = remove_cvref<Generator>;
   using engine_raw = raw_arithmetic_type<typename engine_type::result_type>;
   using engine_unsigned = make_unsigned_type<engine_raw>;
   engine_unsigned const engine_span =
      static_cast<engine_unsigned>(make_raw_arithmetic(engine_type::max()))
      - static_cast<engine_unsigned>(make_raw_arithmetic(engine_type::min()))
      + 1u;
   if constexpr (limits<engine_unsigned>::digits >= limits<T>::digits) {
      if (engine_span == 0u && is_unsigned<engine_raw>) {
         return random_bounded(generator, bound);
      }
   }

   T const threshold = bound == 0u ? 0u : T(-bound) % bound;
   while (true) {
      T value = 0u;
      for (idx bit = 0u; bit < limits<T>::digits; ++bit) {
         value |= T(distribution_random_bit(generator)) << bit.raw;
      }
      if (value >= threshold) {
         return bound == 0u ? value : value % bound;
      }
   }
}

template <is_integral Int>
constexpr auto
distribution_int_from_bits(make_unsigned_type<raw_arithmetic_type<Int>> value)
   -> Int {
   using raw_type = raw_arithmetic_type<Int>;
   if constexpr (is_signed<raw_type>) {
      return Int(__builtin_bit_cast(raw_type, value));
   } else {
      return Int(value);
   }
}

template <is_floating_point Float>
constexpr auto
distribution_next_toward(Float value, Float toward) -> Float {
   using raw_type = raw_arithmetic_type<Float>;
   using bits_type =
      conditional<sizeof(raw_type) == 4u, __UINT32_TYPE__, __UINT64_TYPE__>;
   raw_type const raw_value = make_raw_arithmetic(value);
   raw_type const raw_toward = make_raw_arithmetic(toward);
   if (raw_value == raw_toward) {
      return toward;
   }
   bits_type bits = __builtin_bit_cast(bits_type, raw_value);
   if (raw_value == raw_type(0)) {
      bits = 1u;
      if (raw_toward < raw_type(0)) {
         bits |= bits_type(1) << (limits<bits_type>::digits - 1u);
      }
   } else if ((raw_value < raw_toward) == (raw_value > raw_type(0))) {
      ++bits;
   } else {
      --bits;
   }
   return Float(__builtin_bit_cast(raw_type, bits));
}

template <is_floating_point Float, uniform_random_bit_generator Generator>
constexpr auto
distribution_generate_canonical(Generator& generator) -> Float {
   __UINT64_TYPE__ value = 0u;
   for (idx bit = 0u; bit < limits<Float>::digits; ++bit) {
      value |= static_cast<__UINT64_TYPE__>(distribution_random_bit(generator))
               << bit.raw;
   }
   __UINT64_TYPE__ const denominator = static_cast<__UINT64_TYPE__>(1)
                                       << limits<Float>::digits;
   using raw_type = raw_arithmetic_type<Float>;
   return Float(
      static_cast<raw_type>(value) / static_cast<raw_type>(denominator)
   );
}

}  // namespace cat::detail
