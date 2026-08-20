// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/math>
#include <cat/random>

// `cat::student_t_distribution` models a standardized mean with `n` degrees
// of freedom, similar to `std::student_t_distribution`. `Float` is the result
// and parameter type and defaults to `float8`. The degrees of freedom are read
// with `.n()`.
//
// This can be used to model standardized estimates when variance is uncertain.
// For example, it can form a confidence interval for a small-sample mean.
//
// Example usage:
//
//    pcg_dxsm_engine<uint8> rng;
//    student_t_distribution<float8> student_t(8);
//    float8 result = student_t(rng);
//
// Unlike `std::student_t_distribution`, this implementation supports SIMD.
// Each degrees-of-freedom lane produces its own statistic:
//
//    pcg_dxsm_engine<uint8x2> rng;
//    student_t_distribution<float8x2> simd_student_t(float8x2(8));
//    float8x2 results = simd_student_t(rng);
//
// References
//    https://en.cppreference.com/w/cpp/numeric/random/student_t_distribution

namespace cat {

template <typename Float = float8>
   requires(is_floating_point<Float> || is_simd_floating_point<Float>)
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
      return -limits<Float>::infinity();
   }

   static constexpr auto
   max() -> result_type {
      return limits<Float>::infinity();
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
