// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/integer_distribution_helpers.hpp>

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/math>
#include <cat/random>

namespace cat {

// Reference:
// https://en.cppreference.com/w/cpp/numeric/random/poisson_distribution
// https://www.orange-kiwi.com/posts/fast-integer-poisson-random-variates-for-procedural-generation/
template <is_integral Int = int4, is_floating_point Float = float8>
   requires(!is_bool<Int>)
class poisson_distribution {
 public:
   using result_type = Int;

   class param_type {
    public:
      using distribution_type = poisson_distribution;

      constexpr explicit param_type(Float mean = Float(1)) : m_mean(mean) {
      }

      [[nodiscard]]
      constexpr auto
      mean() const -> Float {
         return m_mean;
      }

      friend constexpr auto
      operator==(param_type const&, param_type const&) -> bool = default;

    private:
      Float m_mean;
   };

   constexpr explicit poisson_distribution(Float mean = Float(1))
       : m_parameters(mean) {
   }

   constexpr explicit poisson_distribution(param_type parameters)
       : m_parameters(parameters) {
   }

   constexpr void
   reset() {
   }

   [[nodiscard]]
   constexpr auto
   mean() const -> Float {
      return m_parameters.mean();
   }

   constexpr auto
   param() const -> param_type {
      return m_parameters;
   }

   constexpr void
   param(param_type parameters) {
      m_parameters = parameters;
   }

   constexpr auto
   min() const -> result_type {
      return 0u;
   }

   constexpr auto
   max() const -> result_type {
      return limits<result_type>::max();
   }

   template <is_uniform_random_bit_generator Generator>
   constexpr auto
   operator()(Generator& generator) -> result_type {
      return (*this)(generator, m_parameters);
   }

   template <is_uniform_random_bit_generator Generator>
   constexpr auto
   operator()(Generator& generator, param_type const& parameters)
      -> result_type {
      Float const mean_value = parameters.mean();
      if (mean_value < detail::distribution_float<Float>(10)) {
         return small_mean(generator, mean_value);
      }
      return transformed_rejection(generator, mean_value);
   }

   friend constexpr auto
   operator==(poisson_distribution const&, poisson_distribution const&)
      -> bool = default;

 private:
   template <is_uniform_random_bit_generator Generator>
   static constexpr auto
   small_mean(Generator& generator, Float mean_value) -> result_type {
      Float const limit = exp(-mean_value);
      Float product = detail::distribution_float<Float>(1);
      result_type result = 0u;
      while (true) {
         product *= detail::distribution_unit<Float>(generator);
         if (product <= limit) {
            return result;
         }
         if (result == limits<result_type>::max()) {
            return result;
         }
         ++result;
      }
   }

   template <is_uniform_random_bit_generator Generator>
   static constexpr auto
   transformed_rejection(Generator& generator, Float mean_value)
      -> result_type {
      auto const f = [](auto value) {
         return detail::distribution_float<Float>(value);
      };
      Float const root = sqrt(mean_value);
      Float const scale = f(0.931) + (f(2.53) * root);
      Float const offset = f(-0.059) + (f(0.02483) * scale);
      Float const fast_acceptance_limit =
         f(0.9277) - (f(3.6224) / (scale - f(2)));
      while (true) {
         Float uniform = detail::distribution_unit<Float>(generator);
         if (uniform < f(0.86) * fast_acceptance_limit) {
            Float const centered = (uniform / fast_acceptance_limit) - f(0.43);
            Float const distance_from_edge =
               f(0.5) - (centered < f(0) ? -centered : centered);
            Float const candidate = floor(
               ((((f(2) * offset) / distance_from_edge) + scale) * centered)
               + mean_value + f(0.445)
            );
            return detail::distribution_integer<result_type>(candidate);
         }

         Float const second_uniform =
            detail::distribution_unit<Float>(generator);
         Float centered;
         if (uniform >= fast_acceptance_limit) {
            centered = second_uniform - f(0.5);
         } else {
            centered = (uniform / fast_acceptance_limit) - f(0.93);
            centered = (centered < f(0) ? -f(0.5) : f(0.5)) - centered;
            uniform = second_uniform * fast_acceptance_limit;
         }

         Float const distance_from_edge =
            f(0.5) - (centered < f(0) ? -centered : centered);
         if (
            distance_from_edge == f(0)
            || (distance_from_edge < f(0.013) && uniform > distance_from_edge)
         ) {
            continue;
         }
         Float const candidate = floor(
            ((((f(2) * offset) / distance_from_edge) + scale) * centered)
            + mean_value + f(0.445)
         );
         Float const inverse_alpha = f(1.1239) + (f(1.1328) / (scale - f(3.4)));
         uniform =
            (uniform * inverse_alpha)
            / ((offset / (distance_from_edge * distance_from_edge)) + scale);
         if (candidate >= f(10)) {
            Float const left = log(uniform * root);
            Float right = ((candidate + f(0.5)) * log(mean_value / candidate))
                          - mean_value - log(sqrt(tau<Float>)) + candidate;
            right -=
               ((f(1) / f(12)) - (f(1) / (f(360) * candidate * candidate)))
               / candidate;
            if (left <= right) {
               return detail::distribution_integer<result_type>(candidate);
            }
         } else if (
            candidate >= f(0)
            && log(uniform) < (candidate * log(mean_value)) - mean_value
                                 - lgamma(candidate + f(1))
         ) {
            return detail::distribution_integer<result_type>(candidate);
         }
      }
   }

   param_type m_parameters;
};

}  // namespace cat
