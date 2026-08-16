// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/math>
#include <cat/random>

namespace cat {

// https://en.cppreference.com/w/cpp/numeric/random/cauchy_distribution
template <is_floating_point Float = float8>
class cauchy_distribution {
 public:
   using result_type = Float;

   class param_type {
    public:
      using distribution_type = cauchy_distribution;

      constexpr explicit param_type(Float location = 0.f, Float scale = 1.f)
          : m_location(location), m_scale(scale) {
      }

      constexpr auto
      a() const -> Float {
         return m_location;
      }

      constexpr auto
      b() const -> Float {
         return m_scale;
      }

      friend constexpr auto
      operator==(param_type const&, param_type const&) -> bool = default;

    private:
      Float m_location;
      Float m_scale;
   };

   constexpr explicit cauchy_distribution(
      Float location = 0.f, Float scale = 1.f
   )
       : m_parameter(location, scale) {
   }

   constexpr explicit cauchy_distribution(param_type const& parameter)
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
      return -limits<Float>::max();
   }

   static constexpr auto
   max() -> result_type {
      return limits<Float>::max();
   }

   template <is_uniform_random_bit_generator Generator>
   constexpr auto
   operator()(Generator& generator) -> result_type {
      return (*this)(generator, m_parameter);
   }

   template <is_uniform_random_bit_generator Generator>
   constexpr auto
   operator()(Generator& generator, param_type const& parameter)
      -> result_type {
      Float const uniform = generate_canonical<Float>(generator);
      return parameter.a() + parameter.b() * tan(pi<Float> * (uniform - 0.5));
   }

   friend constexpr auto
   operator==(cauchy_distribution const&, cauchy_distribution const&)
      -> bool = default;

 private:
   param_type m_parameter;
};

}  // namespace cat
