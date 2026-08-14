// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/math>
#include <cat/random>

namespace cat {

// https://en.cppreference.com/w/cpp/numeric/random/normal_distribution
template <is_floating_point Float = float8>
class normal_distribution {
 public:
   using result_type = Float;

   class param_type {
    public:
      using distribution_type = normal_distribution;

      constexpr explicit param_type(
         Float mean = 0.f, Float standard_deviation = 1.f
      )
          : m_mean(mean), m_standard_deviation(standard_deviation) {
      }

      constexpr auto
      mean() const -> Float {
         return m_mean;
      }

      constexpr auto
      stddev() const -> Float {
         return m_standard_deviation;
      }

      friend constexpr auto
      operator==(param_type const&, param_type const&) -> bool = default;

    private:
      Float m_mean;
      Float m_standard_deviation;
   };

   constexpr explicit normal_distribution(
      Float mean = 0.f, Float standard_deviation = 1.f
   )
       : m_parameter(mean, standard_deviation) {
   }

   constexpr explicit normal_distribution(param_type const& parameter)
       : m_parameter(parameter) {
   }

   constexpr void
   reset() {
      m_has_spare = false;
   }

   constexpr auto
   mean() const -> Float {
      return m_parameter.mean();
   }

   constexpr auto
   stddev() const -> Float {
      return m_parameter.stddev();
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

   template <uniform_random_bit_generator Generator>
   constexpr auto
   operator()(Generator& generator) -> result_type {
      return (*this)(generator, m_parameter);
   }

   template <uniform_random_bit_generator Generator>
   constexpr auto
   operator()(Generator& generator, param_type const& parameter)
      -> result_type {
      Float standard;
      if (m_has_spare) {
         m_has_spare = false;
         standard = m_spare;
      } else {
         constexpr Float tau_value = tau<Float>;
         Float const radius = sqrt(
            -2.f * log(detail::random_positive_canonical<Float>(generator))
         );
         Float const angle = tau_value * generate_canonical<Float>(generator);
         standard = radius * cos(angle);
         m_spare = radius * cos(angle - tau_value / 4.f);
         m_has_spare = true;
      }
      return parameter.mean() + parameter.stddev() * standard;
   }

   friend constexpr auto
   operator==(normal_distribution const& left, normal_distribution const& right)
      -> bool {
      return left.m_parameter == right.m_parameter
             && left.m_has_spare == right.m_has_spare
             && (!left.m_has_spare || left.m_spare == right.m_spare);
   }

 private:
   param_type m_parameter;
   Float m_spare = 0.f;
   bool m_has_spare = false;
};

}  // namespace cat
