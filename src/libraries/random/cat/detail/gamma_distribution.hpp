// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/random>

namespace cat {

// https://en.cppreference.com/w/cpp/numeric/random/gamma_distribution
template <is_floating_point Float = float8>
class gamma_distribution {
 public:
   using result_type = Float;

   class param_type {
    public:
      using distribution_type = gamma_distribution;

      constexpr explicit param_type(Float alpha = 1.f, Float beta = 1.f)
          : m_alpha(alpha), m_beta(beta) {
      }

      constexpr auto
      alpha() const -> Float {
         return m_alpha;
      }

      constexpr auto
      beta() const -> Float {
         return m_beta;
      }

      friend constexpr auto
      operator==(param_type const&, param_type const&) -> bool = default;

    private:
      Float m_alpha;
      Float m_beta;
   };

   constexpr explicit gamma_distribution(Float alpha = 1.f, Float beta = 1.f)
       : m_parameter(alpha, beta) {
   }

   constexpr explicit gamma_distribution(param_type const& parameter)
       : m_parameter(parameter) {
   }

   constexpr void
   reset() {
   }

   constexpr auto
   alpha() const -> Float {
      return m_parameter.alpha();
   }

   constexpr auto
   beta() const -> Float {
      return m_parameter.beta();
   }

   constexpr auto
   param() const -> param_type {
      return m_parameter;
   }

   constexpr void
   param(param_type const& parameter) {
      m_parameter = parameter;
   }

   static constexpr auto
   min() -> result_type {
      return 0.f;
   }

   static constexpr auto
   max() -> result_type {
      return limits<Float>::max();
   }

   template <uniform_random_bit_generator Generator>
   constexpr auto
   operator()(Generator& generator) -> result_type {
      return (*this)(generator, m_parameter);
   }

   template <uniform_random_bit_generator Generator>
   constexpr auto
   operator()(Generator& generator, param_type const& parameter)
      -> result_type {
      return detail::random_gamma(
         generator, parameter.alpha(), parameter.beta()
      );
   }

   friend constexpr auto
   operator==(gamma_distribution const&, gamma_distribution const&)
      -> bool = default;

 private:
   param_type m_parameter;
};

}  // namespace cat
