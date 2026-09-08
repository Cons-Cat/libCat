// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/math>
#include <cat/meta>
#include <cat/random>
#include <cat/simd>
#include <cat/simd_ops>

#include "./random_batch.hpp"

namespace cat::detail {

template <typename Distribution>
struct distribution_batch_traits;

template <
   typename Abi, typename Distribution,
   is_uniform_random_bit_generator Generator>
[[nodiscard]]
constexpr auto
generate_random_distribution_batch(
   Distribution& distribution, Generator& generator
) -> decltype(auto) {
   return distribution_batch_traits<Distribution>::template generate<Abi>(
      distribution, generator
   );
}

template <typename T, typename Abi>
using distribution_batch_simd =
   simd<T, ::cat::simd_abi::deduce<T, Abi::lanes, Abi>>;

template <typename T>
struct random_scalar_type {
   using type = T;
};

template <typename T, typename Abi>
struct random_scalar_type<simd<T, Abi>> {
   using type = T;
};

template <typename T>
using random_scalar = random_scalar_type<T>::type;

template <
   typename Generator, typename Abi,
   typename Result = random_scalar<typename Generator::result_type>>
class distribution_batch_engine {
 public:
   using result_type = distribution_batch_simd<Result, Abi>;

   constexpr explicit distribution_batch_engine(Generator& generator)
       : m_generator(generator) {
   }

   [[nodiscard]]
   constexpr auto
   operator()() -> result_type {
      auto const source = [&] {
         if constexpr (is_simd<typename Generator::result_type>) {
            return m_generator();
         } else {
            using source_abi = distribution_batch_simd<
               typename Generator::result_type, Abi>::abi_type;
            return generate_exact_random_batch<source_abi>(m_generator);
         }
      }();
      if constexpr (is_same<decltype(source), result_type>) {
         return source;
      } else {
         return result_type(
            __builtin_convertvector(source.raw, typename result_type::raw_type)
         );
      }
   }

   [[nodiscard]]
   static constexpr auto
   min() -> result_type {
      return result_type(limits<Result>::min());
   }

   [[nodiscard]]
   static constexpr auto
   max() -> result_type {
      return result_type(limits<Result>::max());
   }

 private:
   Generator& m_generator;
};

template <typename T, typename Shape>
struct random_rebind_type {
   using type = T;
};

template <typename T, typename U, typename Abi>
struct random_rebind_type<T, simd<U, Abi>> {
   using type = rebind_simd<T, simd<U, Abi>>;
};

template <typename T, typename Shape>
using random_rebind = random_rebind_type<T, Shape>::type;

template <typename T>
using random_unsigned = random_rebind<make_unsigned_type<random_scalar<T>>, T>;

template <typename Int, is_floating_point Float>
using distribution_float_result = conditional<
   is_simd<Int>,
   random_rebind<
      conditional<sizeof(random_scalar<Int>) <= 4u, float4, float8>, Int>,
   Float>;

template <typename Int>
using distribution_sampling_float = random_rebind<
   conditional<sizeof(random_scalar<Int>) <= 4u, float4, float8>, Int>;

// TODO: Remove these once scalar and SIMD conversions share an API.
template <typename T>
constexpr auto
distribution_float(auto value) -> T {
   using scalar = random_scalar<T>;
   return T(scalar(value));
}

template <typename T, typename U>
constexpr auto
distribution_broadcast(U value) -> T {
   using scalar = random_scalar<T>;
   return T(scalar(make_raw_arithmetic(value)));
}

// TODO: Remove these once scalar and SIMD mask operations share an API.
template <typename Mask>
constexpr auto
distribution_any(Mask mask) -> bool {
   if constexpr (is_simd_mask<Mask>) {
      return simd_any_of(mask);
   } else {
      return mask;
   }
}

template <typename Mask, typename T>
constexpr auto
distribution_select(Mask mask, T on_true, T on_false) -> T {
   if constexpr (is_simd_mask<Mask>) {
      return simd_select(mask, on_true, on_false);
   } else {
      return mask ? on_true : on_false;
   }
}

template <is_simd To, is_simd_mask SimdMask>
constexpr auto
distribution_mask_cast(SimdMask mask) -> To::mask_type {
   using result = To::mask_type;
   static_assert(SimdMask::abi_type::lanes == result::abi_type::lanes);
   return result(__builtin_bit_cast(typename result::raw_type, mask.raw));
}

template <typename T>
constexpr auto
distribution_equal_lanes(T left, T right) {
   if constexpr (is_simd<T>) {
      return left.equal_lanes(right);
   } else {
      return left == right;
   }
}

// TODO: Remove when `cat::pow` accepts SIMD arguments.
template <typename Float>
   requires(is_floating_point<Float> || is_simd_floating_point<Float>)
constexpr auto
distribution_pow(Float base, Float exponent) -> Float {
   if constexpr (is_simd<Float>) {
      return simd_pow(base, exponent);
   } else {
      return pow(base, exponent);
   }
}

template <is_uniform_random_bit_generator Generator>
constexpr auto
distribution_engine_word(Generator& generator) {
   using engine_type = __typeof_unqual(generator);
   using unsigned_type = random_unsigned<typename engine_type::result_type>;
   unsigned_type const minimum = engine_type::min();
   return unsigned_type(generator()) - minimum;
}

template <
   is_simd_unsigned_integral T, is_uniform_random_bit_generator Generator>
constexpr auto
distribution_random_word(Generator& generator) -> T {
   using engine_type = __typeof_unqual(generator);
   using engine_result = engine_type::result_type;
   using engine_unsigned = random_unsigned<engine_result>;
   using engine_lane = engine_unsigned::value_type;
   using lane_type = T::value_type;
   using mask_type = engine_unsigned::mask_type;
   static_assert(is_simd<engine_result>);
   static_assert(engine_result::abi_type::lanes == T::abi_type::lanes);
   static_assert(sizeof(engine_lane) == sizeof(lane_type));

   engine_unsigned const maximum = engine_type::max();
   engine_unsigned const minimum = engine_type::min();
   engine_unsigned const span = maximum - minimum + 1u;
   auto const full_range = span.equal_lanes(engine_unsigned(0u));
   if (full_range.all_of()) {
      return T(distribution_engine_word(generator));
   }

   engine_unsigned const divisor =
      simd_select(full_range, engine_unsigned(2u), span);
   engine_unsigned const limit = divisor - divisor % 2u;
   T result = 0u;
   for (idx bit = 0u; bit < limits<lane_type>::digits; ++bit) {
      mask_type pending(true);
      while (distribution_any(pending)) {
         engine_unsigned const value = distribution_engine_word(generator);
         auto const accepted = pending && (full_range || value < limit);
         T const selected = T(value & 1u) << lane_type(bit.raw);
         result = simd_select(accepted, result | selected, result);
         pending = pending && !accepted;
      }
   }
   return result;
}

template <is_uniform_random_bit_generator Generator>
constexpr auto
distribution_random_bit(Generator& generator) -> bool {
   using engine_type = __typeof_unqual(generator);
   static_assert(!is_simd<typename engine_type::result_type>);
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
   using engine_type = __typeof_unqual(generator);
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

template <typename Int, typename Unsigned>
   requires(is_integral<Int> || is_simd_integral<Int>)
constexpr auto
distribution_int_from_bits(Unsigned value) -> Int {
   static_assert(is_same<Unsigned, random_unsigned<Int>>);
   if constexpr (is_simd<Int>) {
      return Int(__builtin_bit_cast(typename Int::raw_type, value.raw));
   } else if constexpr (is_signed<Int>) {
      return Int(
         __builtin_bit_cast(
            raw_arithmetic_type<Int>, make_raw_arithmetic(value)
         )
      );
   } else {
      return Int(value);
   }
}

template <typename Float>
   requires(is_floating_point<Float> || is_simd_floating_point<Float>)
constexpr auto
distribution_next_toward(Float value, Float toward) -> Float {
   using scalar = random_scalar<Float>;
   using raw_type = raw_arithmetic_type<scalar>;
   using bits_scalar = uint_fixed<sizeof(raw_type)>;
   using bits_type = random_rebind<bits_scalar, Float>;
   if constexpr (is_simd<Float>) {
      using bits_mask = bits_type::mask_type;
      bits_type bits(
         __builtin_bit_cast(typename bits_type::raw_type, value.raw)
      );
      bits_type const one = 1u;
      bits_type const sign = bits_type(1u)
                             << (limits<bits_scalar>::digits - 1u);
      bits_mask const zero(
         __builtin_bit_cast(
            typename bits_mask::raw_type, value.equal_lanes(Float(0)).raw
         )
      );
      bits_mask const negative_toward(
         __builtin_bit_cast(
            typename bits_mask::raw_type, (toward < Float(0)).raw
         )
      );
      bits_type const zero_bits = simd_select(negative_toward, one | sign, one);
      bits_mask const increment(
         __builtin_bit_cast(
            typename bits_mask::raw_type,
            ((value < toward) == (value > Float(0))).raw
         )
      );
      bits_type const stepped = simd_select(increment, bits + one, bits - one);
      bits = simd_select(zero, zero_bits, stepped);
      return Float(__builtin_bit_cast(typename Float::raw_type, bits.raw));
   } else {
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
}

template <typename Float, is_uniform_random_bit_generator Generator>
   requires(is_floating_point<Float> || is_simd_floating_point<Float>)
constexpr auto
distribution_generate_canonical(Generator& generator) -> Float {
   using engine_result = Generator::result_type;
   using engine_scalar = random_scalar<engine_result>;
   using unsigned_scalar = make_unsigned_type<engine_scalar>;
   using scalar = random_scalar<Float>;
   using raw_float = raw_arithmetic_type<scalar>;
   if constexpr (is_simd<Float>) {
      static_assert(is_simd<engine_result>);
      static_assert(engine_result::abi_type::lanes == Float::abi_type::lanes);
   } else {
      static_assert(!is_simd<engine_result>);
   }
   constexpr unsigned_scalar minimum = [] {
      if constexpr (is_simd<engine_result>) {
         return unsigned_scalar(Generator::min()[0u]);
      } else {
         return unsigned_scalar(Generator::min());
      }
   }();
   constexpr unsigned_scalar maximum = [] {
      if constexpr (is_simd<engine_result>) {
         return unsigned_scalar(Generator::max()[0u]);
      } else {
         return unsigned_scalar(Generator::max());
      }
   }();
   constexpr unsigned_scalar span_minus_one = maximum - minimum;
   constexpr unsigned_scalar span = span_minus_one + 1u;
   constexpr idx digits = limits<scalar>::digits;
   constexpr idx engine_bits = limits<unsigned_scalar>::digits;
   constexpr bool power_of_two = span == 0u || (span & span_minus_one) == 0u;
   constexpr idx radix_bits =
      span == 0u ? engine_bits : idx(bit_width(span_minus_one));
   constexpr idx calls = random_canonical_calls<digits, engine_bits>(span);
   auto const to_float = [](auto word) -> Float {
      if constexpr (is_simd<Float>) {
         using float_word = random_rebind<scalar, engine_result>;
         return Float(float_word(
            __builtin_convertvector(word.raw, typename float_word::raw_type)
         ));
      } else {
         return Float(raw_float(word));
      }
   };
   if constexpr (power_of_two && radix_bits >= digits) {
      auto const word = distribution_engine_word(generator);
      auto const shifted = [&] {
         if constexpr (is_simd<engine_result>) {
            return word >> random_unsigned<engine_result>(
                      engine_scalar((radix_bits - digits).raw)
                   );
         } else {
            return word >> (radix_bits - digits).raw;
         }
      }();
      return to_float(shifted)
             * scalar(sizeof(scalar) == 4u ? 0x1p-24f : 0x1p-53);
   }
   Float value = 0.f;
   Float factor = 1.f;
   Float const radix = Float(raw_float(span_minus_one) + raw_float(1.f));
   for (idx index = 0u; index < calls; ++index) {
      value += to_float(distribution_engine_word(generator)) * factor;
      factor *= radix;
   }
   value /= factor;
   Float const one = 1.f;
   return distribution_select(
      value < one, value, distribution_next_toward(one, Float(0.f))
   );
}

template <typename Float, is_uniform_random_bit_generator Generator>
   requires(is_floating_point<Float> || is_simd_floating_point<Float>)
constexpr auto
random_positive_canonical(Generator& generator) -> Float {
   Float value = distribution_generate_canonical<Float>(generator);
   if constexpr (is_simd<Float>) {
      auto zero = value.equal_lanes(Float(0));
      while (distribution_any(zero)) {
         Float const redraw = distribution_generate_canonical<Float>(generator);
         value = simd_select(zero, redraw, value);
         zero = value.equal_lanes(Float(0));
      }
   } else {
      while (value == 0.f) {
         value = distribution_generate_canonical<Float>(generator);
      }
   }
   return value;
}

template <typename Float, is_uniform_random_bit_generator Generator>
   requires(is_floating_point<Float> || is_simd_floating_point<Float>)
constexpr auto
random_standard_normal(Generator& generator) -> Float {
   using scalar = random_scalar<Float>;
   Float const radius =
      sqrt(Float(-2.f) * log(random_positive_canonical<Float>(generator)));
   Float const angle =
      Float(tau<scalar>) * distribution_generate_canonical<Float>(generator);
   return radius * raw_cos(angle);
}

template <typename Float, is_uniform_random_bit_generator Generator>
   requires(is_floating_point<Float> || is_simd_floating_point<Float>)
constexpr auto
random_gamma(Generator& generator, Float alpha, Float beta) -> Float {
   Float const one = 1.f;
   if constexpr (is_simd<Float>) {
      auto const small_shape = alpha < one;
      Float const shape = simd_select(small_shape, alpha + one, alpha);
      Float const shape_offset = shape - one / 3.f;
      Float const normal_scale = one / sqrt(Float(9.f) * shape_offset);
      Float result{};
      typename Float::mask_type pending(true);
      while (distribution_any(pending)) {
         Float const normal = random_standard_normal<Float>(generator);
         Float const factor = one + normal_scale * normal;
         Float const factor_cubed = factor * factor * factor;
         Float const uniform = random_positive_canonical<Float>(generator);
         Float const normal_squared = normal * normal;
         auto const accepted =
            pending && factor > Float(0)
            && (uniform < one - Float(0.0331f) * normal_squared * normal_squared || log(uniform) < normal_squared / 2.f + shape_offset * (one - factor_cubed + log(factor_cubed)));
         result =
            simd_select(accepted, beta * shape_offset * factor_cubed, result);
         pending = pending && !accepted;
      }
      Float const adjusted =
         result
         * distribution_pow(
            random_positive_canonical<Float>(generator), one / alpha
         );
      return simd_select(small_shape, adjusted, result);
   } else {
      if (alpha < 1.f) {
         Float const value = random_gamma(generator, alpha + 1.f, beta);
         Float const uniform = random_positive_canonical<Float>(generator);
         return value * pow(uniform, one / alpha);
      }
      Float const shape_offset = alpha - one / 3.f;
      Float const normal_scale = one / sqrt(9.f * shape_offset);
      while (true) {
         Float const normal = random_standard_normal<Float>(generator);
         Float const factor = 1.f + normal_scale * normal;
         if (factor <= 0.f) {
            continue;
         }
         Float const factor_cubed = factor * factor * factor;
         Float const uniform = random_positive_canonical<Float>(generator);
         Float const normal_squared = normal * normal;
         if (
            uniform < 1.f - 0.0331f * normal_squared * normal_squared
            || log(uniform)
                  < normal_squared / 2.f
                       + shape_offset * (1.f - factor_cubed + log(factor_cubed))
         ) {
            return beta * shape_offset * factor_cubed;
         }
      }
   }
}

}  // namespace cat::detail
