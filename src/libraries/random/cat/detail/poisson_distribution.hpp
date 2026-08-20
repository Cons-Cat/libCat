// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/limits>
#include <cat/math>
#include <cat/random>

#include "./distribution_helpers.hpp"
#include "./integer_distribution_helpers.hpp"

// `cat::poisson_distribution` models the number of independent events in a
// fixed interval with expected count `mean`, similar to
// `std::poisson_distribution`. `Int` is the result type and defaults to
// `int4`. `Float` stores the mean and defaults to `float8`. The parameter is
// read with `.mean()`.
//
// This can be used to count independent events in a fixed interval. For
// example, it can model calls arriving within one minute.
//
// Example usage:
//
//    pcg_dxsm_engine<uint8> rng;
//    poisson_distribution<int4, float8> poisson(4);
//    int4 result = poisson(rng);
//
// Unlike `std::poisson_distribution`, this implementation supports SIMD.
// Each engine lane produces its own count using the shared mean:
//
//    pcg_dxsm_engine<uint4x4> rng;
//    poisson_distribution<int4x4> simd_poisson(4);
//    int4x4 results = simd_poisson(rng);
//
// References
//    https://en.cppreference.com/w/cpp/numeric/random/poisson_distribution
//    https://www.orange-kiwi.com/posts/fast-integer-poisson-random-variates-for-procedural-generation/

namespace cat {

template <typename Int = int4, is_floating_point Float = float8>
   requires((is_integral<Int> || is_simd_integral<Int>) && !is_bool<Int>)
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
      if constexpr (is_simd<result_type>) {
         using sample_type =
            detail::distribution_float_result<result_type, Float>;
         return vector_sample(
            generator,
            detail::distribution_broadcast<sample_type>(parameters.mean())
         );
      } else {
         Float const mean_value = parameters.mean();
         if (mean_value < detail::distribution_float<Float>(10)) {
            return small_mean(generator, mean_value);
         }
         return transformed_rejection(generator, mean_value);
      }
   }

   friend constexpr auto
   operator==(poisson_distribution const&, poisson_distribution const&)
      -> bool = default;

 private:
   // TODO: Poisson is one of my most important distributions so I should do
   // deeper research/experimentation on quality implementation.
   template <is_uniform_random_bit_generator Generator, typename Sample>
      requires(is_floating_point<Sample> || is_simd_floating_point<Sample>)
   static constexpr auto
   vector_sample(Generator& generator, Sample mean_value) -> result_type {
      Sample output{};
      auto const small = mean_value < Sample(10.f);
      auto pending = small;
      Sample product = 1.f;
      Sample count = 0.f;
      Sample const limit = exp(-mean_value);
      Sample const maximum = detail::distribution_float<Sample>(
         make_raw_arithmetic(limits<detail::random_scalar<result_type>>::max())
      );
      while (detail::distribution_any(pending)) {
         Sample const next_product =
            product * detail::distribution_unit<Sample>(generator);
         auto const finished =
            pending && (next_product <= limit || count.equal_lanes(maximum));
         output = simd_select(finished, count, output);
         product = simd_select(pending, next_product, product);
         count += simd_select(pending && !finished, Sample(1.f), Sample(0.f));
         pending = pending && !finished;
      }

      auto const large = !small;
      pending = large;
      Sample const root = sqrt(mean_value);
      Sample const scale = Sample(0.931f) + Sample(2.53f) * root;
      Sample const offset = Sample(-0.059f) + Sample(0.02483f) * scale;
      Sample const fast_limit =
         Sample(0.9277f) - Sample(3.6224f) / (scale - Sample(2.f));
      while (detail::distribution_any(pending)) {
         Sample uniform = detail::distribution_unit<Sample>(generator);
         Sample const second = detail::distribution_unit<Sample>(generator);
         Sample const fast_centered = uniform / fast_limit - Sample(0.43f);
         Sample const fast_distance =
            Sample(0.5f)
            - simd_select(
               fast_centered < Sample(0.f), -fast_centered, fast_centered
            );
         Sample const fast_candidate = floor(
            ((Sample(2.f) * offset / fast_distance + scale) * fast_centered)
            + mean_value + Sample(0.445f)
         );
         auto const fast = pending && uniform < Sample(0.86f) * fast_limit;
         output = simd_select(fast, fast_candidate, output);
         pending = pending && !fast;

         Sample centered = simd_select(
            uniform >= fast_limit, second - Sample(0.5f),
            uniform / fast_limit - Sample(0.93f)
         );
         auto const tail = uniform < fast_limit;
         centered = simd_select(
            tail,
            simd_select(centered < Sample(0.f), Sample(-0.5f), Sample(0.5f))
               - centered,
            centered
         );
         uniform = simd_select(tail, second * fast_limit, uniform);
         Sample const distance =
            Sample(0.5f)
            - simd_select(centered < Sample(0.f), -centered, centered);
         auto eligible = pending && !distance.equal_lanes(Sample(0.f))
                         && !(distance < Sample(0.013f) && uniform > distance);
         Sample const candidate = floor(
            ((Sample(2.f) * offset / distance + scale) * centered) + mean_value
            + Sample(0.445f)
         );
         Sample const inverse_alpha =
            Sample(1.1239f) + Sample(1.1328f) / (scale - Sample(3.4f));
         uniform =
            uniform * inverse_alpha / (offset / (distance * distance) + scale);

         Sample const left = log(uniform * root);
         Sample right = (candidate + Sample(0.5f)) * log(mean_value / candidate)
                        - mean_value
                        - log(sqrt(Sample(tau<detail::random_scalar<Sample>>)))
                        + candidate;
         right -= (Sample(1.f) / Sample(12.f)
                   - Sample(1.f) / (Sample(360.f) * candidate * candidate))
                  / candidate;
         auto const accept_large = candidate >= Sample(10.f) && left <= right;

         Sample log_factorial{};
         for (idx factor = 2u; factor < 10u; ++factor) {
            Sample const factor_value =
               detail::random_scalar<Sample>(factor.raw);
            log_factorial += simd_select(
               candidate >= factor_value, log(factor_value), Sample(0.f)
            );
         }
         auto const accept_small =
            candidate >= Sample(0.f) && candidate < Sample(10.f)
            && log(uniform)
                  < candidate * log(mean_value) - mean_value - log_factorial;
         auto const accepted = eligible && (accept_large || accept_small);
         output = simd_select(accepted, candidate, output);
         pending = pending && !accepted;
      }
      return detail::distribution_integer<result_type>(output);
   }

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
