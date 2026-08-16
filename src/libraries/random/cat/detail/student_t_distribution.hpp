// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/math>
#include <cat/random>

namespace cat {

// https://en.cppreference.com/w/cpp/numeric/random/student_t_distribution
template <is_floating_point Float = float8>
class student_t_distribution {
 public:
   using result_type = Float;

   class param_type {
    public:
      using distribution_type = student_t_distribution;

      constexpr explicit param_type(Float degrees_of_freedom = 1.f)
          : m_degrees_of_freedom(degrees_of_freedom) {
      }

      constexpr auto
      n() const -> Float {
         return m_degrees_of_freedom;
      }

      friend constexpr auto
      operator==(param_type const&, param_type const&) -> bool = default;

    private:
      Float m_degrees_of_freedom;
   };

   constexpr explicit student_t_distribution(Float degrees_of_freedom = 1.f)
       : m_parameter(degrees_of_freedom) {
   }

   constexpr explicit student_t_distribution(param_type const& parameter)
       : m_parameter(parameter) {
   }

   constexpr void
   reset() {
   }

   constexpr auto
   n() const -> Float {
      return m_parameter.n();
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
      Float const normal = detail::random_standard_normal<Float>(generator);
      Float const chi_squared =
         detail::random_gamma<Float>(generator, parameter.n() / 2.f, 2.f);
      return normal / sqrt(chi_squared / parameter.n());
   }

   friend constexpr auto
   operator==(student_t_distribution const&, student_t_distribution const&)
      -> bool = default;

 private:
   param_type m_parameter;
};

}  // namespace cat
