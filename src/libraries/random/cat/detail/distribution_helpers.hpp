// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/meta>
#include <cat/random>

namespace cat::detail {

template <is_uniform_random_bit_generator Generator>
constexpr auto
distribution_engine_word(Generator& generator) {
   using engine_type = typeof_unqual(generator);
   using unsigned_type = make_unsigned_type<typename engine_type::result_type>;
   unsigned_type const minimum = engine_type::min();
   return unsigned_type(generator()) - minimum;
}

template <is_uniform_random_bit_generator Generator>
constexpr auto
distribution_random_bit(Generator& generator) -> bool {
   using engine_type = typeof_unqual(generator);
   using unsigned_type = make_unsigned_type<typename engine_type::result_type>;
   unsigned_type const maximum = engine_type::max();
   unsigned_type const minimum = engine_type::min();
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

template <is_uniform_random_bit_generator Generator, is_unsigned_integral T>
constexpr auto
distribution_random_bounded(Generator& generator, T bound) -> T {
   using engine_type = typeof_unqual(generator);
   using engine_unsigned =
      make_unsigned_type<typename engine_type::result_type>;
   engine_unsigned const engine_span = engine_unsigned(engine_type::max())
                                       - engine_unsigned(engine_type::min())
                                       + 1u;
   if constexpr (limits<engine_unsigned>::digits >= limits<T>::digits) {
      if (engine_span == 0u && is_unsigned<typename engine_type::result_type>) {
         return random_bounded(generator, bound);
      }
   }

   auto const next_word = [&] {
      T value = 0u;
      for (idx bit = 0u; bit < limits<T>::digits; ++bit) {
         value |= T(distribution_random_bit(generator)) << bit.raw;
      }
      return value;
   };
   if (bound == 0u) {
      return next_word();
   }
   return lemire_bounded(bound, next_word);
}

template <is_integral Int>
constexpr auto
distribution_int_from_bits(make_unsigned_type<Int> value) -> Int {
   if constexpr (is_signed<Int>) {
      return Int(
         __builtin_bit_cast(
            raw_arithmetic_type<Int>, make_raw_arithmetic(value)
         )
      );
   } else {
      return Int(value);
   }
}

template <is_floating_point Float>
constexpr auto
distribution_next_toward(Float value, Float toward) -> Float {
   using raw_type = raw_arithmetic_type<Float>;
   using bits_type = conditional<sizeof(raw_type) == 4u, uint4, uint8>;
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

template <is_floating_point Float, is_uniform_random_bit_generator Generator>
constexpr auto
distribution_generate_canonical(Generator& generator) -> Float {
   uint8 value = 0u;
   for (idx bit = 0u; bit < limits<Float>::digits; ++bit) {
      value |= uint8(distribution_random_bit(generator)) << bit.raw;
   }
   uint8 const denominator = uint8(1) << limits<Float>::digits;
   using raw_type = raw_arithmetic_type<Float>;
   return Float(raw_type(value) / raw_type(denominator));
}

}  // namespace cat::detail
