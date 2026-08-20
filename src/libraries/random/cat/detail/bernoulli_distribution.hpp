// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/meta>
#include <cat/random>
#include <cat/simd>

#include "./distribution_helpers.hpp"

// `cat::bernoulli_distribution` models a true/false experiment, similar to
// `std::bernoulli_distribution`. It returns true with probability `p` and false
// with probability `1 - p`. `p` is set in its constructor and defaults to 0.5.
// This can be read later by `.p()`.
//
// This can be used to model binary outcomes with a chosen probability. For
// example, it can model weighted or unweighted coin flips.
//
// Example usage:
//
//    pcg_dxsm_engine<uint8> rng;
//    auto bernoulli = bernoulli_distribution(0.5f);
//    bool result = bernoulli(rng);  // 50% chance of true, 50% chance of false.
//
// Unlike `std::bernoulli_distribution`, this implementation supports SIMD.
// Each probability lane produces its own boolean result:
//
//    pcg_dxsm_engine<uint8x4> rng;
//    auto bernoulli =
//       bernoulli_distribution(float8x4{0.1, 0.25, 0.5, 0.75});
//    auto results = bernoulli(rng);
//
//    auto result0 = results[0u];  // 10% chance of true, 90% chance of false.
//    auto result1 = results[1u];  // 25% chance of true, 75% chance of false.
//    auto result2 = results[2u];  // 50% chance of true, 50% chance of false.
//    auto result3 = results[3u];  // 75% chance of true, 25% chance of false.
//
// References
//    https://en.cppreference.com/w/cpp/numeric/random/bernoulli_distribution

namespace cat {

template <typename Float = float8>
   requires(is_floating_point<Float> || is_simd_floating_point<Float>)
class bernoulli_distribution {
 public:
   using result_type = decltype(Float{} < Float{});

   class param_type {
    public:
      using distribution_type = bernoulli_distribution;

      constexpr explicit param_type(Float probability = Float(0.5f))
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
      return result_type(false);
   }

   static constexpr auto
   max() -> result_type {
      return result_type(true);
   }

   constexpr auto
   param() const -> param_type {
      return m_parameters;
   }

   constexpr void
   param(param_type const& parameters) {
      m_parameters = parameters;
   }

   // TODO: Consider using `detail::convenience_engine` by default to
   // distributions' call operators to simplify use.

   template <is_uniform_random_bit_generator Generator>
   constexpr auto
   operator()(Generator& generator) const -> result_type {
      return (*this)(generator, m_parameters);
   }

   template <is_uniform_random_bit_generator Generator>
   constexpr auto
   operator()(Generator& generator, param_type const& parameters) const
      -> result_type {
      return detail::distribution_generate_canonical<Float>(generator)
             < parameters.p();
   }

   friend constexpr auto
   operator==(bernoulli_distribution const&, bernoulli_distribution const&)
      -> bool = default;

 private:
   param_type m_parameters;
};

}  // namespace cat
