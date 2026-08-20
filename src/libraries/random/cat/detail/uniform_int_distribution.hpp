// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/meta>
#include <cat/random>
#include <cat/simd>

#include "./distribution_helpers.hpp"

// `cat::uniform_int_distribution` models a discrete uniform integer on the
// inclusive interval [`a`, `b`], similar to `std::uniform_int_distribution`.
// Every representable value in between is equally likely to be returned. The
// interval bounds are set in the constructor and read with `.a()` and `.b()`.
//
// This can be used to choose an integer uniformly from a bounded range. For
// example, it can model a fair die roll.
//
// Example usage:
//
//    pcg_dxsm_engine<uint8> rng;
//    uniform_int_distribution<int4> dice(1, 6);
//    int4 result = dice(rng);
//
// Unlike `std::uniform_int_distribution`, this implementation supports SIMD.
// Each bounds lane pair produces its own integer result:
//
//    pcg_dxsm_engine<uint4x4> rng;
//    uniform_int_distribution<int4x4> simd_dice(int4x4(1), int4x4(6));
//    int4x4 results = simd_dice(rng);
//
// References
//    https://en.cppreference.com/w/cpp/numeric/random/uniform_int_distribution

namespace cat {

template <typename Int = int4>
   requires((is_integral<Int> || is_simd_integral<Int>) && !is_bool<Int>)
class uniform_int_distribution {
 public:
   using result_type = Int;

   class param_type {
    public:
      using distribution_type = uniform_int_distribution;

      constexpr explicit param_type(
         result_type lower = 0, result_type upper = limits<result_type>::max()
      )
          : m_lower(lower), m_upper(upper) {
      }

      constexpr auto
      a() const -> result_type {
         return m_lower;
      }

      constexpr auto
      b() const -> result_type {
         return m_upper;
      }

      friend constexpr auto
      operator==(param_type const&, param_type const&) -> bool = default;

    private:
      result_type m_lower;
      result_type m_upper;
   };

   constexpr uniform_int_distribution() = default;

   constexpr explicit uniform_int_distribution(
      result_type lower, result_type upper = limits<result_type>::max()
   )
       : m_parameters(lower, upper) {
   }

   constexpr explicit uniform_int_distribution(param_type const& parameters)
       : m_parameters(parameters) {
   }

   constexpr void
   reset() {
   }

   constexpr auto
   a() const -> result_type {
      return m_parameters.a();
   }

   constexpr auto
   b() const -> result_type {
      return m_parameters.b();
   }

   constexpr auto
   min() const -> result_type {
      return a();
   }

   constexpr auto
   max() const -> result_type {
      return b();
   }

   constexpr auto
   param() const -> param_type {
      return m_parameters;
   }

   constexpr void
   param(param_type const& parameters) {
      m_parameters = parameters;
   }

   template <is_uniform_random_bit_generator Generator>
   constexpr auto
   operator()(Generator& generator) const -> result_type {
      return (*this)(generator, m_parameters);
   }

   template <is_uniform_random_bit_generator Generator>
   constexpr auto
   operator()(Generator& generator, param_type const& parameters) const
      -> result_type {
      using unsigned_type = detail::random_unsigned<result_type>;
      if constexpr (is_simd<result_type>) {
         using generator_result = typename Generator::result_type;
         static_assert(is_simd<generator_result>);
         static_assert(
            generator_result::abi_type::lanes == result_type::abi_type::lanes
         );
         unsigned_type const lower(
            __builtin_bit_cast(
               typename unsigned_type::raw_type, parameters.a().raw
            )
         );
         unsigned_type const upper(
            __builtin_bit_cast(
               typename unsigned_type::raw_type, parameters.b().raw
            )
         );
         unsigned_type const bound = upper - lower + 1u;
         auto const full_range = bound.equal_lanes(unsigned_type(0u));
         unsigned_type const divisor =
            simd_select(full_range, unsigned_type(1u), bound);
         unsigned_type const threshold = (unsigned_type(0u) - bound) % divisor;
         unsigned_type offset{};
         typename unsigned_type::mask_type pending(true);
         while (detail::distribution_any(pending)) {
            unsigned_type const value =
               detail::distribution_engine_word(generator);
            auto const accepted = pending && (full_range || value >= threshold);
            unsigned_type const candidate =
               simd_select(full_range, value, value % divisor);
            offset = simd_select(accepted, candidate, offset);
            pending = pending && !accepted;
         }
         return detail::distribution_int_from_bits<result_type>(lower + offset);
      } else {
         unsigned_type const lower = unsigned_type(parameters.a());
         unsigned_type const upper = unsigned_type(parameters.b());
         unsigned_type const bound = upper - lower + 1u;
         unsigned_type const offset =
            detail::distribution_random_bounded(generator, bound);
         return detail::distribution_int_from_bits<result_type>(lower + offset);
      }
   }

   friend constexpr auto
   operator==(uniform_int_distribution const&, uniform_int_distribution const&)
      -> bool = default;

 private:
   param_type m_parameters;
};

}  // namespace cat
