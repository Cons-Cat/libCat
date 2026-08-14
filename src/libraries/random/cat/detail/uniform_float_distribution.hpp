// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/distribution_helpers.hpp>
#include <cat/detail/independent_simd_engine.hpp>

#include <cat/arithmetic>
#include <cat/meta>
#include <cat/random>
#include <cat/simd>

namespace cat {

// https://en.cppreference.com/w/cpp/numeric/random/uniform_real_distribution
// https://eel.is/c++draft/rand.dist.uni.real
template <is_floating_point Float = float8>
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

   template <uniform_random_bit_generator Generator>
   constexpr auto
   operator()(Generator& generator) const -> result_type {
      return (*this)(generator, m_parameters);
   }

   template <uniform_random_bit_generator Generator>
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
      if (result >= upper) {
         result = detail::distribution_next_toward(upper, lower);
      }
      if (result < lower) {
         result = lower;
      }
      return result;
   }

   template <uniform_random_bit_generator Engine, is_simd Simd>
      requires is_same<typename Simd::value_type, typename Engine::result_type>
   constexpr auto
   operator()(independent_simd_engine<Engine, Simd>& engine) const
      -> fixed_size_simd<result_type, Simd::abi_type::lanes> {
      return (*this)(engine, m_parameters);
   }

   template <uniform_random_bit_generator Engine, is_simd Simd>
      requires is_same<typename Simd::value_type, typename Engine::result_type>
   constexpr auto
   operator()(
      independent_simd_engine<Engine, Simd>& engine,
      param_type const& parameters
   ) const -> fixed_size_simd<result_type, Simd::abi_type::lanes> {
      fixed_size_simd<result_type, Simd::abi_type::lanes> result;
      for (idx lane = 0u; lane < Simd::abi_type::lanes; ++lane) {
         result.set_lane(lane, (*this)(engine.lane(lane), parameters));
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
