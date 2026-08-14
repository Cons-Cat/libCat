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

// https://en.cppreference.com/w/cpp/numeric/random/bernoulli_distribution
// https://eel.is/c++draft/rand.dist.bern.bernoulli
template <is_floating_point Float = float8>
class bernoulli_distribution {
 public:
   using result_type = bool;

   class param_type {
    public:
      using distribution_type = bernoulli_distribution;

      constexpr explicit param_type(Float probability = 0.5)
          : m_probability(probability) {
      }

      constexpr auto
      p() const -> Float {
         return m_probability;
      }

      friend constexpr auto
      operator==(param_type const&, param_type const&) -> bool = default;

    private:
      Float m_probability;
   };

   constexpr bernoulli_distribution() = default;

   constexpr explicit bernoulli_distribution(Float probability)
       : m_parameters(probability) {
   }

   constexpr explicit bernoulli_distribution(param_type const& parameters)
       : m_parameters(parameters) {
   }

   constexpr void
   reset() {
   }

   constexpr auto
   p() const -> Float {
      return m_parameters.p();
   }

   static constexpr auto
   min() -> result_type {
      return false;
   }

   static constexpr auto
   max() -> result_type {
      return true;
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
      return detail::distribution_generate_canonical<Float>(generator)
             < parameters.p();
   }

   template <uniform_random_bit_generator Engine, is_simd Simd>
      requires is_same<typename Simd::value_type, typename Engine::result_type>
   constexpr auto
   operator()(independent_simd_engine<Engine, Simd>& engine) const
      -> fixed_size_simd_mask<
         typename Simd::value_type, Simd::abi_type::lanes> {
      return (*this)(engine, m_parameters);
   }

   template <uniform_random_bit_generator Engine, is_simd Simd>
      requires is_same<typename Simd::value_type, typename Engine::result_type>
   constexpr auto
   operator()(
      independent_simd_engine<Engine, Simd>& engine,
      param_type const& parameters
   ) const
      -> fixed_size_simd_mask<
         typename Simd::value_type, Simd::abi_type::lanes> {
      fixed_size_simd_mask<typename Simd::value_type, Simd::abi_type::lanes>
         result;
      for (idx lane = 0u; lane < Simd::abi_type::lanes; ++lane) {
         result.set_lane(lane, (*this)(engine.lane(lane), parameters));
      }
      return result;
   }

   friend constexpr auto
   operator==(bernoulli_distribution const&, bernoulli_distribution const&)
      -> bool = default;

 private:
   param_type m_parameters;
};

}  // namespace cat
