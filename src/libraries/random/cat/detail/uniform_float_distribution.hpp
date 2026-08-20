// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/meta>
#include <cat/random>
#include <cat/simd>

#include "./distribution_helpers.hpp"

// `cat::uniform_float_distribution` models a continuous uniform value on the
// half-open interval [`a`, `b`), similar to `std::uniform_real_distribution`.
// `Float` is the result type and defaults to `float8`. The bounds are set in
// the constructor and read with `.a()` and `.b()`.
//
// This can be used to sample a continuous value uniformly within an interval.
// For example, it can model an arrival time within a fixed time window.
//
// Example usage:
//
//    pcg_dxsm_engine<uint8> rng;
//    uniform_float_distribution<float8> unit(0, 1);
//    float8 result = unit(rng);
//
// Unlike `std::uniform_real_distribution`, this implementation supports SIMD.
// Each bounds lane pair produces its own floating-point result:
//
//    pcg_dxsm_engine<uint8x2> rng;
//    uniform_float_distribution<float8x2> units(float8x2(0), float8x2(1));
//    float8x2 results = units(rng);
//
// References
//    https://en.cppreference.com/w/cpp/numeric/random/uniform_real_distribution

namespace cat {

template <typename Float = float8>
   requires(is_floating_point<Float> || is_simd_floating_point<Float>)
class uniform_float_distribution {
 public:
   using result_type = Float;

   class param_type {
    public:
      using distribution_type = uniform_float_distribution;

      constexpr explicit param_type(
         result_type lower = 0, result_type upper = 1
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

   constexpr uniform_float_distribution() = default;

   constexpr explicit uniform_float_distribution(
      result_type lower, result_type upper = 1
   )
       : m_parameters(lower, upper) {
   }

   constexpr explicit uniform_float_distribution(param_type const& parameters)
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
      result_type const lower = parameters.a();
      result_type const upper = parameters.b();
      if (lower == upper) {
         return lower;
      }
      result_type const unit =
         detail::distribution_generate_canonical<result_type>(generator);
      result_type result = (result_type(1) - unit) * lower + unit * upper;
      if constexpr (is_simd<result_type>) {
         result = simd_select(
            result >= upper, detail::distribution_next_toward(upper, lower),
            result
         );
         result = simd_select(result < lower, lower, result);
      } else {
         if (result >= upper) {
            result = detail::distribution_next_toward(upper, lower);
         }
         if (result < lower) {
            result = lower;
         }
      }
      return result;
   }

   friend constexpr auto
   operator==(
      uniform_float_distribution const&, uniform_float_distribution const&
   ) -> bool = default;

 private:
   param_type m_parameters;
};

}  // namespace cat
