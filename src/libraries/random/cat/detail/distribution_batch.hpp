// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/random>

#include "./distribution_helpers.hpp"

namespace cat::detail {

template <template <typename> typename Distribution, typename Scalar>
struct unary_distribution_batch_traits {
   template <
      typename Abi, is_uniform_random_bit_generator Generator,
      typename... Parameters>
   [[nodiscard]]
   static constexpr auto
   generate_with(Generator& generator, Parameters... parameters) {
      using result = distribution_batch_simd<Scalar, Abi>;
      distribution_batch_engine<Generator, Abi> batch_engine(generator);
      Distribution<result> distribution{result(parameters)...};
      return distribution(batch_engine);
   }
};

template <typename Float>
struct distribution_batch_traits<bernoulli_distribution<Float>> {
   template <typename Abi, is_uniform_random_bit_generator Generator>
   [[nodiscard]]
   static constexpr auto
   generate(bernoulli_distribution<Float>& distribution, Generator& generator) {
      using result = distribution_batch_simd<Float, Abi>;
      distribution_batch_engine<Generator, Abi> batch_engine(generator);
      return bernoulli_distribution<result>(result(distribution.p()))(
         batch_engine
      );
   }
};

template <typename Int, typename Float>
struct distribution_batch_traits<binomial_distribution<Int, Float>> {
   template <typename Abi, is_uniform_random_bit_generator Generator>
   [[nodiscard]]
   static constexpr auto
   generate(
      binomial_distribution<Int, Float>& distribution, Generator& generator
   ) {
      using result = distribution_batch_simd<Int, Abi>;
      using engine_result = make_unsigned_type<Int>;
      distribution_batch_engine<Generator, Abi, engine_result> batch_engine(
         generator
      );
      return binomial_distribution<
         result, Float>(result(distribution.t()), distribution.p())(
         batch_engine
      );
   }
};

template <typename Float>
struct distribution_batch_traits<cauchy_distribution<Float>>
    : unary_distribution_batch_traits<cauchy_distribution, Float> {
   template <typename Abi, is_uniform_random_bit_generator Generator>
   [[nodiscard]]
   static constexpr auto
   generate(cauchy_distribution<Float>& distribution, Generator& generator) {
      return distribution_batch_traits::template generate_with<Abi>(
         generator, distribution.a(), distribution.b()
      );
   }
};

template <typename Float>
struct distribution_batch_traits<chi_squared_distribution<Float>>
    : unary_distribution_batch_traits<chi_squared_distribution, Float> {
   template <typename Abi, is_uniform_random_bit_generator Generator>
   [[nodiscard]]
   static constexpr auto
   generate(
      chi_squared_distribution<Float>& distribution, Generator& generator
   ) {
      return distribution_batch_traits::template generate_with<Abi>(
         generator, distribution.n()
      );
   }
};

template <typename Int>
struct distribution_batch_traits<discrete_distribution<Int>> {
   template <typename Abi, is_uniform_random_bit_generator Generator>
   [[nodiscard]]
   static constexpr auto
   generate(discrete_distribution<Int>& distribution, Generator& generator) {
      using result = distribution_batch_simd<Int, Abi>;
      using engine_result = make_unsigned_type<Int>;
      distribution_batch_engine<Generator, Abi, engine_result> batch_engine(
         generator
      );
      return discrete_distribution<result>(distribution.probabilities())(
         batch_engine
      );
   }
};

template <typename Float>
struct distribution_batch_traits<exponential_distribution<Float>>
    : unary_distribution_batch_traits<exponential_distribution, Float> {
   template <typename Abi, is_uniform_random_bit_generator Generator>
   [[nodiscard]]
   static constexpr auto
   generate(
      exponential_distribution<Float>& distribution, Generator& generator
   ) {
      return distribution_batch_traits::template generate_with<Abi>(
         generator, distribution.lambda()
      );
   }
};

template <typename Float>
struct distribution_batch_traits<extreme_value_distribution<Float>>
    : unary_distribution_batch_traits<extreme_value_distribution, Float> {
   template <typename Abi, is_uniform_random_bit_generator Generator>
   [[nodiscard]]
   static constexpr auto
   generate(
      extreme_value_distribution<Float>& distribution, Generator& generator
   ) {
      return distribution_batch_traits::template generate_with<Abi>(
         generator, distribution.a(), distribution.b()
      );
   }
};

template <typename Float>
struct distribution_batch_traits<fisher_f_distribution<Float>>
    : unary_distribution_batch_traits<fisher_f_distribution, Float> {
   template <typename Abi, is_uniform_random_bit_generator Generator>
   [[nodiscard]]
   static constexpr auto
   generate(fisher_f_distribution<Float>& distribution, Generator& generator) {
      return distribution_batch_traits::template generate_with<Abi>(
         generator, distribution.m(), distribution.n()
      );
   }
};

template <typename Float>
struct distribution_batch_traits<gamma_distribution<Float>>
    : unary_distribution_batch_traits<gamma_distribution, Float> {
   template <typename Abi, is_uniform_random_bit_generator Generator>
   [[nodiscard]]
   static constexpr auto
   generate(gamma_distribution<Float>& distribution, Generator& generator) {
      return distribution_batch_traits::template generate_with<Abi>(
         generator, distribution.alpha(), distribution.beta()
      );
   }
};

template <typename Int, typename Float>
struct distribution_batch_traits<geometric_distribution<Int, Float>> {
   template <typename Abi, is_uniform_random_bit_generator Generator>
   [[nodiscard]]
   static constexpr auto
   generate(
      geometric_distribution<Int, Float>& distribution, Generator& generator
   ) {
      using result = distribution_batch_simd<Int, Abi>;
      using engine_result = make_unsigned_type<Int>;
      distribution_batch_engine<Generator, Abi, engine_result> batch_engine(
         generator
      );
      return geometric_distribution<result, Float>(distribution.p())(
         batch_engine
      );
   }
};

template <typename Float>
struct distribution_batch_traits<lognormal_distribution<Float>>
    : unary_distribution_batch_traits<lognormal_distribution, Float> {
   template <typename Abi, is_uniform_random_bit_generator Generator>
   [[nodiscard]]
   static constexpr auto
   generate(lognormal_distribution<Float>& distribution, Generator& generator) {
      return distribution_batch_traits::template generate_with<Abi>(
         generator, distribution.m(), distribution.s()
      );
   }
};

template <typename Int, typename Float>
struct distribution_batch_traits<negative_binomial_distribution<Int, Float>> {
   template <typename Abi, is_uniform_random_bit_generator Generator>
   [[nodiscard]]
   static constexpr auto
   generate(
      negative_binomial_distribution<Int, Float>& distribution,
      Generator& generator
   ) {
      using result = distribution_batch_simd<Int, Abi>;
      using engine_result = make_unsigned_type<Int>;
      distribution_batch_engine<Generator, Abi, engine_result> batch_engine(
         generator
      );
      return negative_binomial_distribution<
         result, Float>(result(distribution.k()), distribution.p())(
         batch_engine
      );
   }
};

template <typename Float>
struct distribution_batch_traits<normal_distribution<Float>>
    : unary_distribution_batch_traits<normal_distribution, Float> {
   template <typename Abi, is_uniform_random_bit_generator Generator>
   [[nodiscard]]
   static constexpr auto
   generate(normal_distribution<Float>& distribution, Generator& generator) {
      return distribution_batch_traits::template generate_with<Abi>(
         generator, distribution.mean(), distribution.stddev()
      );
   }
};

template <typename Float>
struct distribution_batch_traits<piecewise_constant_distribution<Float>> {
   template <typename Abi, is_uniform_random_bit_generator Generator>
   [[nodiscard]]
   static constexpr auto
   generate(
      piecewise_constant_distribution<Float>& distribution, Generator& generator
   ) {
      using result = distribution_batch_simd<Float, Abi>;
      distribution_batch_engine<Generator, Abi> batch_engine(generator);
      return piecewise_constant_distribution<
         result>(distribution.boundaries(), distribution.densities())(
         batch_engine
      );
   }
};

template <typename Float>
struct distribution_batch_traits<piecewise_linear_distribution<Float>> {
   template <typename Abi, is_uniform_random_bit_generator Generator>
   [[nodiscard]]
   static constexpr auto
   generate(
      piecewise_linear_distribution<Float>& distribution, Generator& generator
   ) {
      using result = distribution_batch_simd<Float, Abi>;
      distribution_batch_engine<Generator, Abi> batch_engine(generator);
      return piecewise_linear_distribution<
         result>(distribution.boundaries(), distribution.densities())(
         batch_engine
      );
   }
};

template <typename Int, typename Float>
struct distribution_batch_traits<poisson_distribution<Int, Float>> {
   template <typename Abi, is_uniform_random_bit_generator Generator>
   [[nodiscard]]
   static constexpr auto
   generate(
      poisson_distribution<Int, Float>& distribution, Generator& generator
   ) {
      using result = distribution_batch_simd<Int, Abi>;
      using engine_result = make_unsigned_type<Int>;
      distribution_batch_engine<Generator, Abi, engine_result> batch_engine(
         generator
      );
      return poisson_distribution<result, Float>(distribution.mean())(
         batch_engine
      );
   }
};

template <typename Float>
struct distribution_batch_traits<student_t_distribution<Float>>
    : unary_distribution_batch_traits<student_t_distribution, Float> {
   template <typename Abi, is_uniform_random_bit_generator Generator>
   [[nodiscard]]
   static constexpr auto
   generate(student_t_distribution<Float>& distribution, Generator& generator) {
      return distribution_batch_traits::template generate_with<Abi>(
         generator, distribution.n()
      );
   }
};

template <typename Float>
struct distribution_batch_traits<uniform_float_distribution<Float>>
    : unary_distribution_batch_traits<uniform_float_distribution, Float> {
   template <typename Abi, is_uniform_random_bit_generator Generator>
   [[nodiscard]]
   static constexpr auto
   generate(
      uniform_float_distribution<Float>& distribution, Generator& generator
   ) {
      return distribution_batch_traits::template generate_with<Abi>(
         generator, distribution.a(), distribution.b()
      );
   }
};

template <typename Int>
struct distribution_batch_traits<uniform_int_distribution<Int>> {
   template <typename Abi, is_uniform_random_bit_generator Generator>
   [[nodiscard]]
   static constexpr auto
   generate(uniform_int_distribution<Int>& distribution, Generator& generator) {
      using result = distribution_batch_simd<Int, Abi>;
      using engine_result = make_unsigned_type<Int>;
      distribution_batch_engine<Generator, Abi, engine_result> batch_engine(
         generator
      );
      return uniform_int_distribution<
         result>(result(distribution.a()), result(distribution.b()))(
         batch_engine
      );
   }
};

template <typename Float>
struct distribution_batch_traits<weibull_distribution<Float>>
    : unary_distribution_batch_traits<weibull_distribution, Float> {
   template <typename Abi, is_uniform_random_bit_generator Generator>
   [[nodiscard]]
   static constexpr auto
   generate(weibull_distribution<Float>& distribution, Generator& generator) {
      return distribution_batch_traits::template generate_with<Abi>(
         generator, distribution.a(), distribution.b()
      );
   }
};

}  // namespace cat::detail
