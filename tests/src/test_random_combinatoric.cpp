#include <cat/random>

#include "../unit_tests.hpp"

namespace {

class scripted_simd_engine {
 public:
   using result_type = cat::uint4x4;

   template <cat::idx size>
   constexpr explicit scripted_simd_engine(
      cat::array<result_type, size> const& words
   )
       : m_words(words.data(), words.size()) {
   }

   static constexpr auto
   min() -> result_type {
      return 0u;
   }

   static constexpr auto
   max() -> result_type {
      return cat::limits<result_type>::max();
   }

   constexpr auto
   operator()() -> result_type {
      cat::verify(m_next < m_words.size());
      result_type const result = m_words[m_next];
      ++m_next;
      return result;
   }

   constexpr auto
   calls() const -> cat::idx {
      return m_next;
   }

 private:
   cat::span<result_type const> m_words;
   cat::idx m_next = 0u;
};

class scripted_scalar_engine {
 public:
   using result_type = cat::uint4;

   template <cat::idx size>
   constexpr explicit scripted_scalar_engine(
      cat::array<cat::uint4x4, size> const& words, cat::idx lane
   )
       : m_words(words.data(), words.size()), m_lane(lane) {
   }

   static constexpr auto
   min() -> result_type {
      return 0u;
   }

   static constexpr auto
   max() -> result_type {
      return cat::limits<result_type>::max();
   }

   constexpr auto
   operator()() -> result_type {
      constexpr cat::idx canonical_bits = cat::limits<cat::float4>::digits;
      cat::idx const word_index = m_calls / canonical_bits;
      cat::idx const bit = m_calls % canonical_bits;
      cat::verify(word_index < m_words.size());
      ++m_calls;
      cat::uint4 const canonical = m_words[word_index][m_lane] >> 8u;
      return (canonical >> bit.raw) & 1u;
   }

 private:
   cat::span<cat::uint4x4 const> m_words;
   cat::idx m_lane;
   cat::idx m_calls = 0u;
};

constexpr void
verify_close(
   cat::float4 actual, cat::float4 expected,
   cat::source_location const& callsite = cat::source_location::current()
) {
   cat::float4 const difference =
      actual < expected ? expected - actual : actual - expected;
   cat::float4 const magnitude = expected < 0.f ? -expected : expected;
   cat::verify(difference <= 0.0002f * (1.f + magnitude), callsite);
}

template <typename Simd>
constexpr void
verify_distinct_lanes(Simd const& values) {
   bool distinct = false;
   for (cat::idx lane = 1u; lane < Simd::abi_type::lanes; ++lane) {
      distinct |= values[lane] != values[0u];
   }
   cat::verify(distinct);
}

auto
make_distribution_words(cat::uint8 seed = 0x7251u)
   -> cat::array<cat::uint4x4, 512u> {
   cat::array<cat::uint4x4, 512u> words;
   cat::xoshiro_engine<cat::uint4x4> source(seed);
   for (auto& word : words) {
      word = source();
   }
   return words;
}

template <typename Engine>
void
draw_engine(Engine& engine) {
   auto const value = engine();
   if constexpr (requires {
                    Engine::min();
                    Engine::max();
                 }) {
      if constexpr (cat::is_simd<cat::remove_cvref<decltype(value)>>) {
         cat::verify(
            cat::simd_all_of(value >= Engine::min() && value <= Engine::max())
         );
      } else {
         cat::verify(value >= Engine::min() && value <= Engine::max());
      }
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
   cat::uniform_int_distribution<Simd> uniform(Simd(0), Simd(7));
   Simd const uniform_values = uniform(engine);
   for (auto value : uniform_values) {
      cat::verify(value >= Int(0) && value <= Int(7));
   }

   cat::discrete_distribution<Simd> discrete{1, 2, 3};
   Simd const discrete_values = discrete(engine);
   for (auto value : discrete_values) {
      cat::verify(value >= Int(0) && value <= Int(2));
   }

   cat::binomial_distribution<Simd> binomial(Simd(8), 0.5);
   Simd const binomial_values = binomial(engine);
   for (auto value : binomial_values) {
      cat::verify(value >= Int(0) && value <= Int(8));
   }

   cat::negative_binomial_distribution<Simd> negative_binomial(Simd(3), 0.5);
   Simd const negative_values = negative_binomial(engine);
   for (auto value : negative_values) {
      cat::verify(value >= Int(0));
   }

   cat::geometric_distribution<Simd> geometric(0.5);
   Simd const geometric_values = geometric(engine);
   for (auto value : geometric_values) {
      cat::verify(value >= Int(0));
   }

   cat::poisson_distribution<Simd> poisson(3);
   Simd const poisson_values = poisson(engine);
   for (auto value : poisson_values) {
      cat::verify(value >= Int(0));
   }
}

template <typename Simd, typename Engine>
void
check_simd_float_distributions(Engine& engine) {
   using Float = Simd::value_type;
   cat::uniform_float_distribution<Simd> uniform(Simd(0), Simd(1));
   Simd const uniform_values = uniform(engine);
   for (auto value : uniform_values) {
      cat::verify(value >= Float(0) && value < Float(1));
   }

   auto const bernoulli =
      cat::bernoulli_distribution<Simd>(Simd(Float(0.5)))(engine);
   static_assert(cat::is_simd_mask<cat::remove_cvref<decltype(bernoulli)>>);
   static_cast<void>(bernoulli);
   static_cast<void>(cat::exponential_distribution<Simd>(Simd(2))(engine));
   static_cast<void>(cat::gamma_distribution<Simd>(Simd(2), Simd(3))(engine));
   static_cast<void>(cat::weibull_distribution<Simd>(Simd(2), Simd(3))(engine));
   static_cast<void>(
      cat::extreme_value_distribution<Simd>(Simd(0), Simd(1))(engine)
   );
   static_cast<void>(cat::normal_distribution<Simd>(Simd(0), Simd(1))(engine));
   Simd const lognormal_values =
      cat::lognormal_distribution<Simd>(Simd(0), Simd(1))(engine);
   for (auto value : lognormal_values) {
      cat::verify(value > Float(0));
   }
   static_cast<void>(cat::chi_squared_distribution<Simd>(Simd(2))(engine));
   static_cast<void>(cat::cauchy_distribution<Simd>(Simd(0), Simd(1))(engine));
   static_cast<void>(
      cat::fisher_f_distribution<Simd>(Simd(2), Simd(3))(engine)
   );
   static_cast<void>(cat::student_t_distribution<Simd>(Simd(3))(engine));

   cat::piecewise_constant_distribution<Simd> constant(
      {Float(0), Float(1), Float(2)}, {Float(1), Float(2)}
   );
   Simd const constant_values = constant(engine);
   for (auto value : constant_values) {
      cat::verify(value >= Float(0) && value < Float(2));
   }

   cat::piecewise_linear_distribution<Simd> linear(
      {Float(0), Float(1), Float(2)}, {Float(1), Float(2), Float(1)}
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
   cat::xoshiro_engine<cat::uint8x2> narrow8(1u);
   draw_engine(wide8);
   draw_engine(wide4);
   draw_engine(narrow8);

   check_simd_integer_distributions<cat::int4x4>(wide4);
   check_simd_integer_distributions<cat::uint4x4>(wide4);
   check_simd_integer_distributions<cat::int8x2>(narrow8);
   check_simd_integer_distributions<cat::uint8x4>(wide8);
   check_simd_float_distributions<cat::float4x4>(wide4);
   check_simd_float_distributions<cat::float8x2>(narrow8);
}

$test(random_simd_distribution_deterministic_lanes) {
   {
      cat::array<cat::uint4x4, 2u> words{
         cat::uint4x4{0u, 1u, 3u,  42u},
         cat::uint4x4{2u, 4u, 10u, 43u},
      };
      scripted_simd_engine engine(words);
      cat::uniform_int_distribution<cat::uint4x4> distribution(
         cat::uint4x4{0u, 10u, 20u, 0u},
         cat::uint4x4{2u, 14u, 26u, cat::uint4::max()}
      );
      cat::uint4x4 const values = distribution(engine);
      cat::verify(values[0u] == 2u);
      cat::verify(values[1u] == 11u);
      cat::verify(values[2u] == 23u);
      cat::verify(values[3u] == 42u);
      cat::verify(engine.calls() == 2u);
   }
   {
      cat::xoshiro_engine<cat::uint4x4> engine(0x913u);
      cat::int4x4 const lower{-8, 0, 100, cat::limits<cat::int4>::min()};
      cat::int4x4 const upper{-3, 0, 130, cat::limits<cat::int4>::max()};
      cat::uniform_int_distribution<cat::int4x4> distribution(lower, upper);
      for (cat::idx draw = 0u; draw < 128u; ++draw) {
         cat::int4x4 const values = distribution(engine);
         for (cat::idx lane = 0u; lane < 4u; ++lane) {
            cat::verify(values[lane] >= lower[lane]);
            cat::verify(values[lane] <= upper[lane]);
         }
         cat::verify(values[1u] == 0);
      }
   }

   cat::array<cat::uint4x4, 5u> words{
      cat::uint4x4{0u, 0x40000000u, 0x80000000u, 0xc0000000u},
      cat::uint4x4{0u, 0x40000000u, 0x80000000u, 0xc0000000u},
      cat::uint4x4{0u, 0x40000000u, 0x80000000u, 0xc0000000u},
      cat::uint4x4{0u, 0x40000000u, 0x80000000u, 0xc0000000u},
      cat::uint4x4{0u, 0x40000000u, 0x80000000u, 0xc0000000u},
   };
   {
      scripted_simd_engine engine(words);
      cat::uniform_float_distribution<cat::float4x4> distribution(
         cat::float4x4{2.f, -4.f, 10.f, 100.f},
         cat::float4x4{2.f, 0.f, 14.f, 108.f}
      );
      cat::verify(
         distribution(engine) == cat::float4x4{2.f, -3.f, 12.f, 106.f}
      );
   }
   {
      scripted_simd_engine engine(words);
      cat::bernoulli_distribution<cat::float4x4> distribution(
         cat::float4x4{0.f, 0.26f, 0.5f, 1.f}
      );
      auto const values = distribution(engine);
      static_assert(
         cat::is_same<decltype(values), cat::float4x4::mask_type const>
      );
      cat::verify(!values[0u]);
      cat::verify(values[1u]);
      cat::verify(!values[2u]);
      cat::verify(values[3u]);
      cat::verify(
         cat::simd_all_of(distribution.min() == cat::float4x4::mask_type(false))
      );
      cat::verify(
         cat::simd_all_of(distribution.max() == cat::float4x4::mask_type(true))
      );
   }
   {
      scripted_simd_engine engine(words);
      cat::geometric_distribution<cat::int4x4, cat::float4> distribution(0.5f);
      cat::verify(distribution(engine) == cat::int4x4{0, 0, 1, 2});
   }
   {
      scripted_simd_engine engine(words);
      cat::negative_binomial_distribution<cat::int4x4, cat::float4>
         distribution(cat::int4x4{0, 1, 2, 4}, 0.5f);
      cat::verify(distribution(engine) == cat::int4x4{0, 0, 2, 8});
   }
   {
      scripted_simd_engine engine(words);
      cat::binomial_distribution<cat::int4x4, cat::float4> distribution(
         cat::int4x4{0, 1, 3, 5}, 1.f
      );
      cat::verify(distribution(engine) == cat::int4x4{0, 1, 3, 5});
   }
   {
      scripted_simd_engine engine(words);
      cat::discrete_distribution<cat::int4x4> distribution{1, 2, 7};
      cat::verify(distribution(engine) == cat::int4x4{0, 1, 2, 2});
   }
   {
      scripted_simd_engine engine(words);
      cat::piecewise_constant_distribution<cat::float4x4> distribution(
         {0.f, 10.f, 20.f}, {1.f, 1.f}
      );
      cat::verify(distribution(engine) == cat::float4x4{0.f, 5.f, 10.f, 15.f});
   }
   {
      scripted_simd_engine engine(words);
      cat::piecewise_linear_distribution<cat::float4x4> distribution(
         {0.f, 10.f, 20.f}, {1.f, 1.f, 1.f}
      );
      cat::verify(distribution(engine) == cat::float4x4{0.f, 5.f, 10.f, 15.f});
   }

   cat::array<cat::uint4x4, 1u> curved_words{
      cat::uint4x4{0u, 0x40000000u, 0x90000000u, 0xc4000000u}
   };
   scripted_simd_engine curved_engine(curved_words);
   cat::piecewise_linear_distribution<cat::float4x4> curved(
      {0.f, 2.f}, {0.f, 1.f}
   );
   cat::float4x4 const curved_values = curved(curved_engine);
   verify_close(curved_values[0u], 0.f);
   verify_close(curved_values[1u], 1.f);
   verify_close(curved_values[2u], 1.5f);
   verify_close(curved_values[3u], 1.75f);

   scripted_scalar_engine scalar_curved_engine(curved_words, 0u);
   cat::verify(
      cat::piecewise_linear_distribution<cat::float4>({0.f, 2.f}, {0.f, 1.f})(
         scalar_curved_engine
      )
      == 0.f
   );
}

$test(random_simd_distribution_scalar_parity) {
   auto const words = make_distribution_words();

   {
      cat::float4x4 const lower{-3.f, -1.f, 2.f, 10.f};
      cat::float4x4 const upper{-2.f, 2.f, 7.f, 18.f};
      scripted_simd_engine engine(words);
      cat::float4x4 const values =
         cat::uniform_float_distribution<cat::float4x4>(lower, upper)(engine);
      for (cat::idx lane = 0u; lane < 4u; ++lane) {
         scripted_scalar_engine scalar_engine(words, lane);
         cat::float4 const expected = cat::uniform_float_distribution<
            cat::float4>(lower[lane], upper[lane])(scalar_engine);
         verify_close(values[lane], expected);
         cat::verify(values[lane] >= lower[lane]);
         cat::verify(values[lane] < upper[lane]);
      }
      verify_distinct_lanes(values);
   }

   {
      cat::float4x4 const probabilities{0.f, 0.2f, 0.7f, 1.f};
      scripted_simd_engine engine(words);
      auto const values =
         cat::bernoulli_distribution<cat::float4x4>(probabilities)(engine);
      static_assert(cat::is_simd_mask<cat::remove_cvref<decltype(values)>>);
      for (cat::idx lane = 0u; lane < 4u; ++lane) {
         scripted_scalar_engine scalar_engine(words, lane);
         bool const expected =
            cat::bernoulli_distribution<cat::float4>(probabilities[lane])(
               scalar_engine
            );
         cat::verify(values[lane] == expected);
      }
      cat::verify(!values[0u]);
      cat::verify(values[3u]);
   }

   {
      cat::int4x4 const trials{0, 2, 5, 9};
      scripted_simd_engine engine(words);
      cat::int4x4 const values =
         cat::binomial_distribution<cat::int4x4, cat::float4>(trials, 0.375f)(
            engine
         );
      for (cat::idx lane = 0u; lane < 4u; ++lane) {
         scripted_scalar_engine scalar_engine(words, lane);
         cat::int4 const expected = cat::binomial_distribution<
            cat::int4, cat::float4>(trials[lane], 0.375f)(scalar_engine);
         cat::verify(values[lane] == expected);
         cat::verify(values[lane] >= 0 && values[lane] <= trials[lane]);
      }
   }

   {
      cat::int4x4 const successes{0, 1, 2, 5};
      scripted_simd_engine engine(words);
      cat::int4x4 const values = cat::negative_binomial_distribution<
         cat::int4x4, cat::float4>(successes, 0.625f)(engine);
      for (cat::idx lane = 0u; lane < 4u; ++lane) {
         scripted_scalar_engine scalar_engine(words, lane);
         cat::int4 const expected = cat::negative_binomial_distribution<
            cat::int4, cat::float4>(successes[lane], 0.625f)(scalar_engine);
         cat::verify(values[lane] == expected);
         cat::verify(values[lane] >= 0);
      }
   }

   {
      scripted_simd_engine engine(words);
      cat::int4x4 const values =
         cat::geometric_distribution<cat::int4x4, cat::float4>(0.35f)(engine);
      for (cat::idx lane = 0u; lane < 4u; ++lane) {
         scripted_scalar_engine scalar_engine(words, lane);
         cat::int4 const expected =
            cat::geometric_distribution<cat::int4, cat::float4>(0.35f)(
               scalar_engine
            );
         cat::verify(values[lane] == expected);
         cat::verify(values[lane] >= 0);
      }
   }

   for (cat::float4 mean : {4.f, 20.f}) {
      scripted_simd_engine engine(words);
      cat::int4x4 const values =
         cat::poisson_distribution<cat::int4x4, cat::float4>(mean)(engine);
      for (cat::idx lane = 0u; lane < 4u; ++lane) {
         scripted_scalar_engine scalar_engine(words, lane);
         cat::int4 const expected =
            cat::poisson_distribution<cat::int4, cat::float4>(mean)(
               scalar_engine
            );
         cat::verify(values[lane] == expected);
         cat::verify(values[lane] >= 0);
      }
      verify_distinct_lanes(values);
   }
   {
      bool exercised_rejection = false;
      for (cat::uint8 seed = 1u; seed <= 64u && !exercised_rejection; ++seed) {
         auto const rejection_words = make_distribution_words(seed);
         scripted_simd_engine engine(rejection_words);
         static_cast<void>(
            cat::poisson_distribution<cat::int4x4, cat::float4>(20.f)(engine)
         );
         exercised_rejection = engine.calls() > 2u;
      }
      cat::verify(exercised_rejection);
   }

   {
      cat::float4x4 const lambda{0.5f, 1.f, 2.f, 4.f};
      scripted_simd_engine engine(words);
      cat::float4x4 const values =
         cat::exponential_distribution<cat::float4x4>(lambda)(engine);
      for (cat::idx lane = 0u; lane < 4u; ++lane) {
         scripted_scalar_engine scalar_engine(words, lane);
         cat::float4 const expected =
            cat::exponential_distribution<cat::float4>(lambda[lane])(
               scalar_engine
            );
         verify_close(values[lane], expected);
         cat::verify(values[lane] >= 0.f);
      }
      verify_distinct_lanes(values);
   }

   {
      cat::float4x4 const alpha{0.5f, 1.f, 2.f, 5.f};
      cat::float4x4 const beta{0.25f, 1.f, 2.f, 4.f};
      scripted_simd_engine engine(words);
      cat::float4x4 const values =
         cat::gamma_distribution<cat::float4x4>(alpha, beta)(engine);
      for (cat::idx lane = 0u; lane < 4u; ++lane) {
         scripted_scalar_engine scalar_engine(words, lane);
         cat::float4 const expected =
            cat::gamma_distribution<cat::float4>(alpha[lane], beta[lane])(
               scalar_engine
            );
         verify_close(values[lane], expected);
         cat::verify(values[lane] >= 0.f);
      }
      verify_distinct_lanes(values);
   }
   {
      bool exercised_rejection = false;
      for (cat::uint8 seed = 1u; seed <= 64u && !exercised_rejection; ++seed) {
         auto const rejection_words = make_distribution_words(seed);
         scripted_simd_engine engine(rejection_words);
         static_cast<void>(
            cat::gamma_distribution<
               cat::
                  float4x4>(cat::float4x4{0.5f, 1.f, 2.f, 5.f}, cat::float4x4{0.25f, 1.f, 2.f, 4.f})(
               engine
            )
         );
         exercised_rejection = engine.calls() > 4u;
      }
      cat::verify(exercised_rejection);
   }

   {
      cat::float4x4 const shape{0.5f, 1.f, 2.f, 4.f};
      cat::float4x4 const scale{0.25f, 1.f, 3.f, 8.f};
      scripted_simd_engine engine(words);
      cat::float4x4 const values =
         cat::weibull_distribution<cat::float4x4>(shape, scale)(engine);
      for (cat::idx lane = 0u; lane < 4u; ++lane) {
         scripted_scalar_engine scalar_engine(words, lane);
         cat::float4 const expected =
            cat::weibull_distribution<cat::float4>(shape[lane], scale[lane])(
               scalar_engine
            );
         verify_close(values[lane], expected);
         cat::verify(values[lane] >= 0.f);
      }
      verify_distinct_lanes(values);
   }

   {
      cat::float4x4 const location{-5.f, -1.f, 2.f, 9.f};
      cat::float4x4 const scale{0.5f, 1.f, 2.f, 4.f};
      scripted_simd_engine engine(words);
      cat::float4x4 const values =
         cat::extreme_value_distribution<cat::float4x4>(location, scale)(
            engine
         );
      for (cat::idx lane = 0u; lane < 4u; ++lane) {
         scripted_scalar_engine scalar_engine(words, lane);
         cat::float4 const expected = cat::extreme_value_distribution<
            cat::float4>(location[lane], scale[lane])(scalar_engine);
         verify_close(values[lane], expected);
      }
      verify_distinct_lanes(values);
   }

   {
      cat::float4x4 const mean{-4.f, -1.f, 2.f, 8.f};
      cat::float4x4 const deviation{0.25f, 0.5f, 1.5f, 3.f};
      scripted_simd_engine engine(words);
      cat::normal_distribution<cat::float4x4> distribution(mean, deviation);
      cat::float4x4 const first = distribution(engine);
      cat::float4x4 const second = distribution(engine);
      cat::idx const calls_after_spare = engine.calls();
      distribution.reset();
      static_cast<void>(distribution(engine));
      cat::verify(engine.calls() > calls_after_spare);
      for (cat::idx lane = 0u; lane < 4u; ++lane) {
         scripted_scalar_engine scalar_engine(words, lane);
         cat::normal_distribution<cat::float4> scalar_distribution(
            mean[lane], deviation[lane]
         );
         verify_close(first[lane], scalar_distribution(scalar_engine));
         verify_close(second[lane], scalar_distribution(scalar_engine));
      }
      verify_distinct_lanes(first);
      verify_distinct_lanes(second);
   }

   {
      cat::float4x4 const mean{-2.f, -1.f, 0.f, 1.f};
      cat::float4x4 const deviation{0.25f, 0.5f, 1.f, 1.5f};
      scripted_simd_engine engine(words);
      cat::float4x4 const values =
         cat::lognormal_distribution<cat::float4x4>(mean, deviation)(engine);
      for (cat::idx lane = 0u; lane < 4u; ++lane) {
         scripted_scalar_engine scalar_engine(words, lane);
         cat::float4 const expected = cat::lognormal_distribution<
            cat::float4>(mean[lane], deviation[lane])(scalar_engine);
         verify_close(values[lane], expected);
         cat::verify(values[lane] > 0.f);
      }
      verify_distinct_lanes(values);
   }

   {
      cat::float4x4 const degrees{0.5f, 1.f, 3.f, 8.f};
      scripted_simd_engine engine(words);
      cat::float4x4 const values =
         cat::chi_squared_distribution<cat::float4x4>(degrees)(engine);
      for (cat::idx lane = 0u; lane < 4u; ++lane) {
         scripted_scalar_engine scalar_engine(words, lane);
         cat::float4 const expected =
            cat::chi_squared_distribution<cat::float4>(degrees[lane])(
               scalar_engine
            );
         verify_close(values[lane], expected);
         cat::verify(values[lane] >= 0.f);
      }
      verify_distinct_lanes(values);
   }

   {
      cat::float4x4 const location{-6.f, -2.f, 1.f, 7.f};
      cat::float4x4 const scale{0.25f, 0.75f, 2.f, 5.f};
      scripted_simd_engine engine(words);
      cat::float4x4 const values =
         cat::cauchy_distribution<cat::float4x4>(location, scale)(engine);
      for (cat::idx lane = 0u; lane < 4u; ++lane) {
         scripted_scalar_engine scalar_engine(words, lane);
         cat::float4 const expected =
            cat::cauchy_distribution<cat::float4>(location[lane], scale[lane])(
               scalar_engine
            );
         verify_close(values[lane], expected);
      }
      verify_distinct_lanes(values);
   }

   {
      cat::float4x4 const numerator{0.5f, 0.8f, 1.2f, 1.8f};
      cat::float4x4 const denominator{0.6f, 1.f, 1.4f, 1.9f};
      scripted_simd_engine engine(words);
      cat::float4x4 const values =
         cat::fisher_f_distribution<cat::float4x4>(numerator, denominator)(
            engine
         );
      for (cat::idx lane = 0u; lane < 4u; ++lane) {
         scripted_scalar_engine scalar_engine(words, lane);
         cat::float4 const expected = cat::fisher_f_distribution<
            cat::float4>(numerator[lane], denominator[lane])(scalar_engine);
         verify_close(values[lane], expected);
         cat::verify(values[lane] >= 0.f);
      }
      verify_distinct_lanes(values);
   }

   {
      cat::float4x4 const degrees{0.5f, 1.f, 3.f, 9.f};
      scripted_simd_engine engine(words);
      cat::float4x4 const values =
         cat::student_t_distribution<cat::float4x4>(degrees)(engine);
      for (cat::idx lane = 0u; lane < 4u; ++lane) {
         scripted_scalar_engine scalar_engine(words, lane);
         cat::float4 const expected =
            cat::student_t_distribution<cat::float4>(degrees[lane])(
               scalar_engine
            );
         verify_close(values[lane], expected);
      }
      verify_distinct_lanes(values);
   }

   {
      scripted_simd_engine engine(words);
      cat::discrete_distribution<cat::int4x4> distribution{1, 3, 2, 6};
      cat::int4x4 const values = distribution(engine);
      for (cat::idx lane = 0u; lane < 4u; ++lane) {
         cat::verify(values[lane] >= 0 && values[lane] <= 3);
      }
      verify_distinct_lanes(values);
   }

   {
      scripted_simd_engine engine(words);
      cat::piecewise_constant_distribution<cat::float4x4> distribution(
         {-4.f, -1.f, 2.f, 8.f}, {1.f, 4.f, 2.f}
      );
      cat::float4x4 const values = distribution(engine);
      for (cat::idx lane = 0u; lane < 4u; ++lane) {
         scripted_scalar_engine scalar_engine(words, lane);
         cat::float4 const expected = cat::piecewise_constant_distribution<
            cat::float4>({-4.f, -1.f, 2.f, 8.f}, {1.f, 4.f, 2.f})(
            scalar_engine
         );
         verify_close(values[lane], expected);
         cat::verify(values[lane] >= -4.f && values[lane] < 8.f);
      }
      verify_distinct_lanes(values);
   }

   {
      scripted_simd_engine engine(words);
      cat::piecewise_linear_distribution<cat::float4x4> distribution(
         {-4.f, -1.f, 2.f, 8.f}, {1.f, 4.f, 2.f, 3.f}
      );
      cat::float4x4 const values = distribution(engine);
      for (cat::idx lane = 0u; lane < 4u; ++lane) {
         scripted_scalar_engine scalar_engine(words, lane);
         cat::float4 const expected = cat::piecewise_linear_distribution<
            cat::float4>({-4.f, -1.f, 2.f, 8.f}, {1.f, 4.f, 2.f, 3.f})(
            scalar_engine
         );
         verify_close(values[lane], expected);
         cat::verify(values[lane] >= -4.f && values[lane] < 8.f);
      }
      verify_distinct_lanes(values);
   }
}
