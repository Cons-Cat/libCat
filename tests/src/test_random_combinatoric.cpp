#include <cat/random>

#include "../unit_tests.hpp"

namespace {

template <typename Engine>
void
draw_engine(Engine& engine) {
   auto const value = engine();
   if constexpr (requires {
                    Engine::min();
                    Engine::max();
                 }) {
      cat::verify(value >= Engine::min() && value <= Engine::max());
   }
   static_cast<void>(value);
}

template <typename Int, typename Engine>
void
check_uniform_int(Engine& engine) {
   cat::uniform_int_distribution<Int> distribution(Int(0), Int(7));
   Int const value = distribution(engine);
   cat::verify(value >= Int(0) && value <= Int(7));
}

template <typename Float, typename Engine>
void
check_uniform_float(Engine& engine) {
   cat::uniform_float_distribution<Float> distribution(Float(0), Float(1));
   Float const value = distribution(engine);
   cat::verify(value >= Float(0) && value < Float(1));
}

template <typename Float, typename Engine>
void
check_bernoulli(Engine& engine) {
   static_cast<void>(cat::bernoulli_distribution<Float>(Float(0.5))(engine));
}

template <typename Int, typename Float, typename Engine>
void
check_binomial(Engine& engine) {
   cat::binomial_distribution<Int, Float> distribution(Int(8), Float(0.5));
   Int const value = distribution(engine);
   cat::verify(value >= Int(0) && value <= Int(8));
}

template <typename Int, typename Float, typename Engine>
void
check_negative_binomial(Engine& engine) {
   cat::negative_binomial_distribution<Int, Float> distribution(
      Int(3), Float(0.5)
   );
   cat::verify(distribution(engine) >= Int(0));
}

template <typename Int, typename Float, typename Engine>
void
check_geometric(Engine& engine) {
   cat::geometric_distribution<Int, Float> distribution(Float(0.5));
   cat::verify(distribution(engine) >= Int(0));
}

template <typename Int, typename Float, typename Engine>
void
check_poisson(Engine& engine) {
   cat::poisson_distribution<Int, Float> distribution(Float(3));
   cat::verify(distribution(engine) >= Int(0));
}

template <typename Float, typename Engine>
void
check_exponential(Engine& engine) {
   cat::verify(
      cat::exponential_distribution<Float>(Float(2))(engine) >= Float(0)
   );
}

template <typename Float, typename Engine>
void
check_gamma(Engine& engine) {
   cat::verify(
      cat::gamma_distribution<Float>(Float(2), Float(3))(engine) >= Float(0)
   );
}

template <typename Float, typename Engine>
void
check_weibull(Engine& engine) {
   cat::verify(
      cat::weibull_distribution<Float>(Float(2), Float(3))(engine) >= Float(0)
   );
}

template <typename Float, typename Engine>
void
check_extreme_value(Engine& engine) {
   static_cast<void>(
      cat::extreme_value_distribution<Float>(Float(0), Float(1))(engine)
   );
}

template <typename Float, typename Engine>
void
check_normal(Engine& engine) {
   static_cast<void>(
      cat::normal_distribution<Float>(Float(0), Float(1))(engine)
   );
}

template <typename Float, typename Engine>
void
check_lognormal(Engine& engine) {
   cat::verify(
      cat::lognormal_distribution<Float>(Float(0), Float(1))(engine) > Float(0)
   );
}

template <typename Float, typename Engine>
void
check_chi_squared(Engine& engine) {
   cat::verify(
      cat::chi_squared_distribution<Float>(Float(2))(engine) >= Float(0)
   );
}

template <typename Float, typename Engine>
void
check_cauchy(Engine& engine) {
   static_cast<void>(
      cat::cauchy_distribution<Float>(Float(0), Float(1))(engine)
   );
}

template <typename Float, typename Engine>
void
check_fisher_f(Engine& engine) {
   cat::verify(
      cat::fisher_f_distribution<Float>(Float(2), Float(3))(engine) >= Float(0)
   );
}

template <typename Float, typename Engine>
void
check_student_t(Engine& engine) {
   static_cast<void>(cat::student_t_distribution<Float>(Float(3))(engine));
}

template <typename Int, typename Engine>
void
check_discrete(Engine& engine) {
   cat::discrete_distribution<Int> distribution{1, 2, 3};
   Int const value = distribution(engine);
   cat::verify(value >= Int(0) && value <= Int(2));
}

template <typename Float, typename Engine>
void
check_piecewise_constant(Engine& engine) {
   cat::piecewise_constant_distribution<Float> distribution(
      {Float(0), Float(1), Float(2)}, {Float(1), Float(2)}
   );
   Float const value = distribution(engine);
   cat::verify(value >= Float(0) && value < Float(2));
}

template <typename Float, typename Engine>
void
check_piecewise_linear(Engine& engine) {
   cat::piecewise_linear_distribution<Float> distribution(
      {Float(0), Float(1), Float(2)}, {Float(1), Float(2), Float(1)}
   );
   Float const value = distribution(engine);
   cat::verify(value >= Float(0) && value < Float(2));
}

template <typename Int, typename Engine>
void
check_integer_distributions(Engine& engine) {
   check_uniform_int<Int>(engine);
   check_discrete<Int>(engine);
}

template <typename Float, typename Engine>
void
check_float_distributions(Engine& engine) {
   check_uniform_float<Float>(engine);
   check_bernoulli<Float>(engine);
   check_exponential<Float>(engine);
   check_gamma<Float>(engine);
   check_weibull<Float>(engine);
   check_extreme_value<Float>(engine);
   check_normal<Float>(engine);
   check_lognormal<Float>(engine);
   check_chi_squared<Float>(engine);
   check_cauchy<Float>(engine);
   check_fisher_f<Float>(engine);
   check_student_t<Float>(engine);
   check_piecewise_constant<Float>(engine);
   check_piecewise_linear<Float>(engine);
   static_cast<void>(cat::generate_canonical<Float>(engine));
}

template <typename Int, typename Float, typename Engine>
void
check_mixed_distributions(Engine& engine) {
   check_binomial<Int, Float>(engine);
   check_negative_binomial<Int, Float>(engine);
   check_geometric<Int, Float>(engine);
   check_poisson<Int, Float>(engine);
}

template <typename Engine>
void
check_all_distribution_types(Engine& engine) {
   check_integer_distributions<cat::int4>(engine);
   check_integer_distributions<cat::int8>(engine);
   check_integer_distributions<int>(engine);
   check_integer_distributions<long>(engine);
   check_integer_distributions<long long>(engine);

   check_float_distributions<cat::float4>(engine);
   check_float_distributions<cat::float8>(engine);
   check_float_distributions<cat::float4_fast>(engine);
   check_float_distributions<cat::float8_fast>(engine);
   check_float_distributions<float>(engine);
   check_float_distributions<double>(engine);

   check_mixed_distributions<cat::int4, cat::float4>(engine);
   check_mixed_distributions<cat::int4, cat::float8>(engine);
   check_mixed_distributions<cat::int4, cat::float4_fast>(engine);
   check_mixed_distributions<cat::int4, cat::float8_fast>(engine);
   check_mixed_distributions<cat::int4, float>(engine);
   check_mixed_distributions<cat::int4, double>(engine);
   check_mixed_distributions<cat::int8, cat::float8>(engine);
   check_mixed_distributions<cat::int8, cat::float8_fast>(engine);
   check_mixed_distributions<cat::int8, double>(engine);
   check_mixed_distributions<int, cat::float4>(engine);
   check_mixed_distributions<int, cat::float8>(engine);
   check_mixed_distributions<int, float>(engine);
   check_mixed_distributions<int, double>(engine);
   check_mixed_distributions<long, cat::float8>(engine);
   check_mixed_distributions<long, double>(engine);
   check_mixed_distributions<long long, cat::float8>(engine);
   check_mixed_distributions<long long, double>(engine);
}

template <typename Engine, typename... Seeds>
void
exercise_engine(Seeds... seeds) {
   Engine engine(seeds...);
   draw_engine(engine);
}

template <typename Word>
void
exercise_word_engines() {
   exercise_engine<cat::xoshiro_engine<Word>>(1u);
   exercise_engine<cat::xoroshiro_engine<Word>>(1u);
   exercise_engine<cat::xoshiro_pp_engine<Word>>(1u);
   exercise_engine<cat::xoroshiro_pp_engine<Word>>(1u);
   exercise_engine<
      cat::xoshiro_engine<Word, cat::detail::xoshiro_scrambler::plus>>(1u);
   if constexpr (sizeof(cat::raw_arithmetic_type<Word>) == 8u) {
      exercise_engine<cat::xoshiro512_engine<Word>>(1u);
      exercise_engine<cat::xoroshiro1024_engine<Word>>(1u);
      exercise_engine<cat::xoshiro512_pp_engine<Word>>(1u);
      exercise_engine<cat::xoroshiro1024_pp_engine<Word>>(1u);
      exercise_engine<
         cat::xoshiro512_engine<Word, cat::detail::xoshiro_scrambler::plus>>(
         1u
      );
   }
}

template <typename Simd, typename Engine>
void
check_simd_integer_distributions(Engine& engine) {
   using Int = Simd::value_type;
   cat::simd_uniform_int_distribution<Simd> uniform(
      cat::uniform_int_distribution<Int>(Int(0), Int(7))
   );
   Simd const uniform_values = uniform(engine);
   for (auto value : uniform_values) {
      cat::verify(value >= Int(0) && value <= Int(7));
   }

   cat::simd_discrete_distribution<Simd> discrete(
      cat::discrete_distribution<Int>{1, 2, 3}
   );
   Simd const discrete_values = discrete(engine);
   for (auto value : discrete_values) {
      cat::verify(value >= Int(0) && value <= Int(2));
   }

   cat::simd_binomial_distribution<Simd> binomial(
      cat::binomial_distribution<Int>(Int(8), 0.5)
   );
   Simd const binomial_values = binomial(engine);
   for (auto value : binomial_values) {
      cat::verify(value >= Int(0) && value <= Int(8));
   }

   cat::simd_negative_binomial_distribution<Simd> negative_binomial(
      cat::negative_binomial_distribution<Int>(Int(3), 0.5)
   );
   Simd const negative_values = negative_binomial(engine);
   for (auto value : negative_values) {
      cat::verify(value >= Int(0));
   }

   cat::simd_geometric_distribution<Simd> geometric(
      cat::geometric_distribution<Int>(0.5)
   );
   Simd const geometric_values = geometric(engine);
   for (auto value : geometric_values) {
      cat::verify(value >= Int(0));
   }

   cat::simd_poisson_distribution<Simd> poisson(
      cat::poisson_distribution<Int>(3)
   );
   Simd const poisson_values = poisson(engine);
   for (auto value : poisson_values) {
      cat::verify(value >= Int(0));
   }
}

template <typename Simd, typename Engine>
void
check_simd_float_distributions(Engine& engine) {
   using Float = Simd::value_type;
   cat::simd_uniform_float_distribution<Simd> uniform(
      cat::uniform_float_distribution<Float>(Float(0), Float(1))
   );
   Simd const uniform_values = uniform(engine);
   for (auto value : uniform_values) {
      cat::verify(value >= Float(0) && value < Float(1));
   }

   static_cast<void>(
      cat::simd_exponential_distribution<
         Simd>(cat::exponential_distribution<Float>(Float(2)))(engine)
   );
   static_cast<void>(
      cat::simd_gamma_distribution<
         Simd>(cat::gamma_distribution<Float>(Float(2), Float(3)))(engine)
   );
   static_cast<void>(
      cat::simd_weibull_distribution<
         Simd>(cat::weibull_distribution<Float>(Float(2), Float(3)))(engine)
   );
   static_cast<void>(
      cat::simd_extreme_value_distribution<
         Simd>(cat::extreme_value_distribution<Float>(Float(0), Float(1)))(
         engine
      )
   );
   static_cast<void>(
      cat::simd_normal_distribution<
         Simd>(cat::normal_distribution<Float>(Float(0), Float(1)))(engine)
   );
   Simd const lognormal_values = cat::simd_lognormal_distribution<
      Simd>(cat::lognormal_distribution<Float>(Float(0), Float(1)))(engine);
   for (auto value : lognormal_values) {
      cat::verify(value > Float(0));
   }
   static_cast<void>(
      cat::simd_chi_squared_distribution<
         Simd>(cat::chi_squared_distribution<Float>(Float(2)))(engine)
   );
   static_cast<void>(
      cat::simd_cauchy_distribution<
         Simd>(cat::cauchy_distribution<Float>(Float(0), Float(1)))(engine)
   );
   static_cast<void>(
      cat::simd_fisher_f_distribution<
         Simd>(cat::fisher_f_distribution<Float>(Float(2), Float(3)))(engine)
   );
   static_cast<void>(
      cat::simd_student_t_distribution<
         Simd>(cat::student_t_distribution<Float>(Float(3)))(engine)
   );

   cat::simd_piecewise_constant_distribution<Simd> constant(
      cat::piecewise_constant_distribution<Float>(
         {Float(0), Float(1), Float(2)}, {Float(1), Float(2)}
      )
   );
   Simd const constant_values = constant(engine);
   for (auto value : constant_values) {
      cat::verify(value >= Float(0) && value < Float(2));
   }

   cat::simd_piecewise_linear_distribution<Simd> linear(
      cat::piecewise_linear_distribution<Float>(
         {Float(0), Float(1), Float(2)}, {Float(1), Float(2), Float(1)}
      )
   );
   Simd const linear_values = linear(engine);
   for (auto value : linear_values) {
      cat::verify(value >= Float(0) && value < Float(2));
   }
}

}  // namespace

$test(random_combinatoric_xoshiro) {
   exercise_word_engines<cat::uint4>();
   exercise_word_engines<cat::uint8>();
   exercise_word_engines<cat::int4>();
   exercise_word_engines<cat::int8>();
   exercise_word_engines<int>();
   exercise_word_engines<unsigned>();
   exercise_word_engines<long>();
   exercise_word_engines<unsigned long>();
   exercise_word_engines<long long>();
   exercise_word_engines<unsigned long long>();
   exercise_word_engines<cat::float4>();
   exercise_word_engines<cat::float8>();
   exercise_word_engines<cat::float4_fast>();
   exercise_word_engines<cat::float8_fast>();
   exercise_word_engines<float>();
   exercise_word_engines<double>();
}

$test(random_combinatoric_pcg_and_mixers) {
   exercise_engine<cat::splitmix64_engine>(1u);
   exercise_engine<cat::wyrand_engine>(1u);

   exercise_engine<cat::pcg_engine<cat::uint4>>(42u, 54u);
   exercise_engine<cat::pcg_engine<cat::uint4, cat::pcg_stream::oneseq>>(42u);
   exercise_engine<cat::pcg_engine<cat::uint4, cat::pcg_stream::unique>>(42u);
   exercise_engine<cat::pcg_engine<cat::uint4, cat::pcg_stream::mcg>>(42u);
   exercise_engine<cat::pcg_engine<cat::uint8>>(42u, 54u);
   exercise_engine<cat::pcg_engine<cat::uint8, cat::pcg_stream::oneseq>>(42u);
   exercise_engine<cat::pcg_engine<cat::uint8, cat::pcg_stream::unique>>(42u);
   exercise_engine<cat::pcg_engine<cat::uint8, cat::pcg_stream::mcg>>(42u);
   exercise_engine<cat::pcg_dxsm_engine<cat::uint4>>(42u, 54u);
   exercise_engine<cat::pcg_dxsm_engine<cat::uint4, cat::pcg_stream::oneseq>>(
      42u
   );
   exercise_engine<cat::pcg_dxsm_engine<cat::uint4, cat::pcg_stream::unique>>(
      42u
   );
   exercise_engine<cat::pcg_dxsm_engine<cat::uint4, cat::pcg_stream::mcg>>(42u);
   exercise_engine<cat::pcg_dxsm_engine<cat::uint8>>(42u, 54u);
   exercise_engine<cat::pcg_dxsm_engine<cat::uint8, cat::pcg_stream::oneseq>>(
      42u
   );
   exercise_engine<cat::pcg_dxsm_engine<cat::uint8, cat::pcg_stream::unique>>(
      42u
   );
   exercise_engine<cat::pcg_dxsm_engine<cat::uint8, cat::pcg_stream::mcg>>(42u);

   exercise_engine<cat::discard_block_engine<cat::wyrand_engine, 5u, 3u>>(9u);
   exercise_engine<
      cat::independent_bits_engine<cat::wyrand_engine, 16u, cat::uint4>>(9u);
   exercise_engine<cat::shuffle_order_engine<cat::wyrand_engine, 16u>>(9u);
}

$test(random_combinatoric_distributions) {
   cat::pcg_engine<cat::uint4> narrow(42u, 54u);
   cat::pcg_engine<cat::uint8> wide(42u, 54u);
   check_all_distribution_types(narrow);
   check_all_distribution_types(wide);
}

$test(random_combinatoric_linux) {
   nix::sys_urandom_engine urandom;
   draw_engine(urandom);

   nix::sys_random_engine blocking;
   draw_engine(blocking);

   nix::dev_urandom_engine dev_urandom;
   draw_engine(dev_urandom);

   nix::dev_random_engine dev_random;
   draw_engine(dev_random);
}

$test(random_combinatoric_simd) {
   cat::xoshiro_engine<cat::uint8x4> wide8(1u);
   cat::xoshiro_engine<cat::uint4x4> wide4(1u);
   cat::xoshiro_engine<cat::float4x4> wide_float(1u);
   draw_engine(wide8);
   draw_engine(wide4);
   draw_engine(wide_float);

   cat::independent_simd_engine<cat::wyrand_engine, cat::uint8x4> independent8(
      1u
   );
   cat::independent_simd_engine<cat::pcg_engine<cat::uint4>, cat::uint4x4>
      independent4(1u);
   draw_engine(independent8);
   draw_engine(independent4);

   cat::pcg_dxsm_engine<cat::uint8> engine(42u, 54u);
   check_simd_integer_distributions<cat::int4x4>(engine);
   check_simd_integer_distributions<cat::int8x2>(engine);
   check_simd_float_distributions<cat::float4x4>(engine);
   check_simd_float_distributions<cat::float8x2>(engine);
   check_simd_integer_distributions<cat::uint8x4>(independent8);
   check_simd_integer_distributions<cat::uint4x4>(independent4);
}
