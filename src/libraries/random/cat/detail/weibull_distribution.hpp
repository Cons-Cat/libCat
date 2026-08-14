// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/math>
#include <cat/random>

namespace cat {

// https://en.cppreference.com/w/cpp/numeric/random/weibull_distribution
template <is_floating_point Float = float8>
class weibull_distribution {
 public:
   using result_type = Float;

   class param_type {
    public:
      using distribution_type = weibull_distribution;

      constexpr explicit param_type(Float shape = 1.f, Float scale = 1.f)
          : m_shape(shape), m_scale(scale) {
      }

      constexpr auto
      a() const -> Float {
         return m_shape;
      }

      constexpr auto
      b() const -> Float {
         return m_scale;
      }

      friend constexpr auto
      operator==(param_type const&, param_type const&) -> bool = default;

    private:
      Float m_shape;
      Float m_scale;
   };

   constexpr explicit weibull_distribution(Float shape = 1.f, Float scale = 1.f)
       : m_parameter(shape, scale) {
   }

   constexpr explicit weibull_distribution(param_type const& parameter)
       : m_parameter(parameter) {
   }

   constexpr void
   reset() {
   }

   constexpr auto
   a() const -> Float {
      return m_parameter.a();
   }

   constexpr auto
   b() const -> Float {
      return m_parameter.b();
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
      Float const exponential =
         -log(detail::random_positive_canonical<Float>(generator));
      Float const one = 1.f;
      return parameter.b() * pow(exponential, one / parameter.a());
   }

   friend constexpr auto
   operator==(weibull_distribution const&, weibull_distribution const&)
      -> bool = default;

 private:
   param_type m_parameter;
};

}  // namespace cat
