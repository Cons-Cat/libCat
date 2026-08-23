#include <cat/bitset>
#include <cat/list>
#include <cat/page_allocator>
#include <cat/raii_vec>
#include <cat/random>
#include <cat/string>
#include <cat/vec_inplace>

#include "../unit_tests.hpp"
#include "cat/debug"

struct random_fill_custom_engine {
   using result_type = cat::uint4;

   bool used = false;

   static constexpr auto
   min() -> cat::uint4 {
      return 0u;
   }

   static constexpr auto
   max() -> cat::uint4 {
      return cat::limits<cat::uint4>::max();
   }

   constexpr auto
   operator()() -> cat::uint4 {
      return 0u;
   }

   template <cat::is_iterable Range>
   constexpr void
   fill_random(Range&& range) {
      used = true;
      for (auto&& value : range) {
         value = 42u;
      }
   }
};

struct random_fill_custom_distribution {
   bool used = false;

   template <cat::is_iterable Range, typename Generator>
   constexpr void
   fill_random(Range&& range, Generator& generator [[maybe_unused]]) {
      used = true;
      for (auto&& value : range) {
         value = 17u;
      }
   }
};

template <typename Container, typename Generator>
concept has_fixed_random_factory =
   requires(Generator& generator) {
      cat::make_filled_random<Container>(generator);
   };

template <typename Container, typename Generator>
concept has_sized_random_factory =
   requires(Generator& generator) {
      cat::make_filled_random<Container>(1u, generator);
   };

template <typename Container, typename Allocator, typename Generator>
concept has_allocator_random_factory =
   requires(Allocator& allocator, Generator& generator) {
      cat::make_filled_random<Container>(
         cat::allocator_ref<Allocator>(allocator), 1u, generator
      );
   };

template <typename Container, typename Allocator, typename Generator>
concept has_bound_allocator_random_factory =
   requires(Allocator& allocator, Generator& generator) {
      cat::raii::make_filled_random<Container>(
         cat::allocator_ref<Allocator>(allocator), 1u, generator
      );
   };

template <typename Container, typename Generator>
concept has_static_random_factory =
   requires(Generator& generator) { Container::make_filled_random(generator); };

using random_factory_engine = cat::xoshiro_engine<cat::uint8>;

static_assert(
   has_fixed_random_factory<cat::array<cat::uint8, 4u>, random_factory_engine>
);
static_assert(has_fixed_random_factory<cat::bitset<4u>, random_factory_engine>);
static_assert(has_sized_random_factory<
              cat::vec_inplace<cat::uint8, 4u>, random_factory_engine>);
static_assert(
   has_sized_random_factory<cat::str_inplace<4u>, random_factory_engine>
);
static_assert(
   has_allocator_random_factory<
      cat::list<cat::uint8>, cat::page_allocator, random_factory_engine>
);
static_assert(has_bound_allocator_random_factory<
              cat::raii::vec<cat::uint8, cat::page_allocator>,
              cat::page_allocator, random_factory_engine>);
static_assert(!has_allocator_random_factory<
              cat::raii::vec<cat::uint8, cat::page_allocator>,
              cat::page_allocator, random_factory_engine>);
static_assert(
   !has_bound_allocator_random_factory<
      cat::list<cat::uint8>, cat::page_allocator, random_factory_engine>
);
static_assert(
   !has_fixed_random_factory<cat::span<cat::uint8>, random_factory_engine>
);
static_assert(
   !has_sized_random_factory<cat::array<cat::uint8, 4u>, random_factory_engine>
);
static_assert(
   !has_static_random_factory<cat::array<cat::uint8, 4u>, random_factory_engine>
);

template <typename Result>
class canonical_counting_engine {
 public:
   using result_type = Result;

   constexpr explicit canonical_counting_engine(result_type value)
       : m_value(value) {
   }

   static constexpr auto
   min() -> result_type {
      return cat::limits<result_type>::min();
   }

   static constexpr auto
   max() -> result_type {
      return cat::limits<result_type>::max();
   }

   constexpr auto
   operator()() -> result_type {
      ++m_calls;
      result_type const result = m_value;
      m_value += 1u;
      return result;
   }

   constexpr auto
   calls() const -> cat::idx {
      return m_calls;
   }

 private:
   result_type m_value;
   cat::idx m_calls = 0u;
};

template <
   typename Result, cat::uint4::raw_type minimum, cat::uint4::raw_type maximum>
class canonical_bounded_engine {
 public:
   using result_type = Result;

   constexpr explicit canonical_bounded_engine(result_type value)
       : m_value(value) {
   }

   static constexpr auto
   min() -> result_type {
      return result_type(minimum);
   }

   static constexpr auto
   max() -> result_type {
      return result_type(maximum);
   }

   constexpr auto
   operator()() -> result_type {
      ++m_calls;
      return m_value;
   }

   constexpr auto
   calls() const -> cat::idx {
      return m_calls;
   }

 private:
   result_type m_value;
   cat::idx m_calls = 0u;
};

class canonical_decimal_engine {
 public:
   using result_type = cat::uint4;

   static constexpr auto
   min() -> result_type {
      return 10u;
   }

   static constexpr auto
   max() -> result_type {
      return 19u;
   }

   constexpr auto
   operator()() -> result_type {
      result_type const result = 10u + result_type(m_calls.raw);
      ++m_calls;
      return result;
   }

   constexpr auto
   calls() const -> cat::idx {
      return m_calls;
   }

 private:
   cat::idx m_calls = 0u;
};

class ternary_engine {
 public:
   using result_type = cat::uint1;

   static constexpr auto
   min() -> result_type {
      return 0u;
   }

   static constexpr auto
   max() -> result_type {
      return 2u;
   }

   constexpr auto
   operator()() -> result_type {
      result_type const result = m_value;
      m_value = m_value == 2u ? result_type(0u) : m_value + 1u;
      ++m_calls;
      return result;
   }

   constexpr auto
   calls() const -> cat::idx {
      return m_calls;
   }

 private:
   result_type m_value = 0u;
   cat::idx m_calls = 0u;
};

template <typename Result>
class simd_binary_engine {
 public:
   using result_type = Result;

   static constexpr auto
   min() -> result_type {
      return result_type(5u);
   }

   static constexpr auto
   max() -> result_type {
      return result_type(6u);
   }

   constexpr auto
   operator()() -> result_type {
      ++m_calls;
      result_type const result = m_value;
      m_value = m_value.equal_lanes(result_type(5u)).all_of() ? result_type(6u)
                                                              : result_type(5u);
      return result;
   }

   constexpr auto
   calls() const -> cat::idx {
      return m_calls;
   }

 private:
   result_type m_value = 5u;
   cat::idx m_calls = 0u;
};

template <typename T, typename Abi>
constexpr bool has_public_random_batch =
   requires(T& value) { value.template batch<Abi>(); };

template <cat::idx lanes, typename Engine, typename... Arguments>
void
verify_sequence_batch(Arguments... arguments) {
   Engine batched(arguments...);
   Engine scalar(arguments...);
   using result_type = Engine::result_type;
   using abi_type = cat::simd_abi::fixed_size<result_type, lanes>;
   auto const values =
      cat::detail::generate_exact_random_batch<abi_type>(batched);
   static_assert(decltype(values)::abi_type::lanes == lanes);
   for (cat::idx lane = 0u; lane < lanes; ++lane) {
      cat::verify(values[lane] == scalar());
   }
   for (cat::idx draw = 0u; draw < 9u; ++draw) {
      cat::verify(batched() == scalar());
   }
}

template <cat::idx count, typename Engine, typename... Arguments>
void
verify_exact_pcg_fill(Arguments... arguments) {
   cat::array<typename Engine::result_type, count> values;
   Engine engine(arguments...);
   Engine expected(arguments...);
   cat::fill_random(values, engine);
   for (auto value : values) {
      cat::verify(value == expected());
   }
   for (cat::idx draw = 0u; draw < 9u; ++draw) {
      cat::verify(engine() == expected());
   }
}

$test(random_seed_strong_type) {
   static_assert(cat::is_convertible<cat::uint4, cat::random_seed>);
   static_assert(cat::is_convertible<unsigned long long, cat::random_seed>);
   static_assert(!cat::is_convertible<cat::random_seed, cat::uint4>);
   static_assert(!cat::is_convertible<cat::random_seed, unsigned long long>);

   cat::random_seed const value = 42u;
   cat::verify(static_cast<cat::uint8>(value) == 42u);
}

$test(random_reference_vectors) {
   cat::splitmix64_engine splitmix(1u);
   cat::verify(splitmix() == 0x910a2dec'89025cc1ull);

   cat::xoshiro_pp_engine<cat::uint8> xoshiro(1u);
   cat::verify(xoshiro() == 0xcfc5d07f'6f03c29bull);

   cat::xoshiro_pp_engine<cat::uint8x4> simd_xoshiro(1u);
   cat::uint8x4 const simd_values = simd_xoshiro();
   cat::verify(simd_values[0u] == 0xcfc5d07f'6f03c29bull);

   cat::pcg_engine<cat::uint4> pcg(42u, 54u);
   cat::verify(pcg() == 0xa15c02b7u);
}

$test(random_engine_state_operations) {
   cat::xoshiro_engine<cat::uint8> left(123u);
   cat::xoshiro_engine<cat::uint8> right(123u);
   cat::verify(left() == right());
   left.jump();
   cat::verify(left() != right());

   cat::pcg_engine<cat::uint4> skipped(123u, 456u);
   cat::pcg_engine<cat::uint4> discarded(123u, 456u);
   skipped.discard(20u);
   for (cat::idx index = 0u; index < 20u; ++index) {
      static_cast<void>(discarded());
   }
   cat::verify(skipped() == discarded());

   cat::splitmix64_engine split_restored(1u);
   split_restored.discard(8u);
   split_restored.backstep(8u);
   cat::verify(split_restored() == cat::splitmix64_engine(1u)());

   cat::discard_block_engine<cat::splitmix64_engine, 5u, 3u> blocked(9u);
   cat::independent_bits_engine<cat::splitmix64_engine, 16u, cat::uint4> bits(
      9u
   );
   cat::shuffle_order_engine<cat::splitmix64_engine, 16u> shuffled(9u);
   cat::verify(blocked() <= blocked.max());
   cat::verify(bits() <= 0xffffu);
   cat::verify(shuffled() <= shuffled.max());

   cat::seed_sequence sequence{1u, 2u, 3u, 4u};
   cat::array<cat::uint4, 8u> seed_words;
   sequence.generate(seed_words);
   cat::verify(seed_words[0u] != seed_words[1u]);
   auto const seed_values = sequence.values();
   cat::verify(seed_values.size() == 4u);
   cat::verify(seed_values[0u] == 1u && seed_values[3u] == 4u);
}

$test(random_internal_scalar_engine_sequence_batches) {
   using abi_type = cat::simd_abi::fixed_size<cat::uint8, 2u>;
   static_assert(!has_public_random_batch<cat::splitmix64_engine, abi_type>);
   static_assert(
      !has_public_random_batch<cat::pcg_dxsm_engine<cat::uint8>, abi_type>
   );
   static_assert(
      !has_public_random_batch<cat::xoshiro_engine<cat::uint8>, abi_type>
   );

   verify_sequence_batch<2u, cat::pcg_engine<cat::uint4>>(42u, 54u);
   verify_sequence_batch<4u, cat::pcg_engine<cat::uint4>>(42u, 54u);
   verify_sequence_batch<8u, cat::pcg_dxsm_engine<cat::uint4>>(42u, 54u);
   verify_sequence_batch<2u, cat::pcg_engine<cat::uint8>>(42u, 54u);
   verify_sequence_batch<4u, cat::pcg_dxsm_engine<cat::uint8>>(42u, 54u);
   verify_sequence_batch<
      4u, cat::pcg_dxsm_engine<cat::uint8, cat::pcg_stream::oneseq>>(42u);
   verify_sequence_batch<
      4u, cat::pcg_dxsm_engine<cat::uint8, cat::pcg_stream::mcg>>(42u);

   verify_sequence_batch<4u, cat::xoshiro_engine<cat::uint4>>(7u);
   verify_sequence_batch<4u, cat::xoshiro_pp_engine<cat::uint4>>(7u);
   verify_sequence_batch<4u, cat::xoroshiro_engine<cat::uint4>>(7u);
   verify_sequence_batch<2u, cat::xoshiro_engine<cat::uint8>>(7u);
   verify_sequence_batch<4u, cat::xoshiro_pp_engine<cat::uint8>>(7u);
   verify_sequence_batch<4u, cat::xoroshiro_engine<cat::uint8>>(7u);
   verify_sequence_batch<4u, cat::xoroshiro_pp_engine<cat::uint8>>(7u);
   verify_sequence_batch<4u, cat::xoshiro512_engine<cat::uint8>>(7u);
   verify_sequence_batch<4u, cat::xoshiro512_pp_engine<cat::uint8>>(7u);
   verify_sequence_batch<4u, cat::xoroshiro1024_engine<cat::uint8>>(7u);
   verify_sequence_batch<4u, cat::xoroshiro1024_pp_engine<cat::uint8>>(7u);
}

$test(random_pcg_exact_bulk_fill) {
   verify_exact_pcg_fill<127u, cat::pcg_engine<cat::uint4>>(42u, 54u);
   verify_exact_pcg_fill<128u, cat::pcg_engine<cat::uint4>>(42u, 54u);
   verify_exact_pcg_fill<1'025u, cat::pcg_engine<cat::uint4>>(42u, 54u);
   verify_exact_pcg_fill<
      1'025u, cat::pcg_engine<cat::uint4, cat::pcg_stream::oneseq>>(42u);
   verify_exact_pcg_fill<
      1'025u, cat::pcg_engine<cat::uint4, cat::pcg_stream::mcg>>(42u);
   verify_exact_pcg_fill<1'025u, cat::pcg_dxsm_engine<cat::uint4>>(42u, 54u);

   using unique_engine = cat::pcg_engine<cat::uint4, cat::pcg_stream::unique>;
   cat::array<cat::uint4, 1'025u> values;
   cat::array<cat::uint4, 1'025u> expected;
   unique_engine unique(42u);
   for (auto& value : expected) {
      value = unique();
   }
   unique.seed(42u);
   cat::fill_random(values, unique);
   cat::verify(values == expected);
}

$test(random_uniform_distributions) {
   cat::xoshiro_engine<cat::uint8> engine(987u);
   cat::uniform_int_distribution<cat::int4> integers(-8, 13);
   cat::uniform_float_distribution<cat::float8> floats(-2, 4);

   for (cat::idx index = 0u; index < 1'000u; ++index) {
      cat::int4 const integer = integers(engine);
      cat::verify(integer >= -8 && integer <= 13);
      cat::float8 const floating = floats(engine);
      cat::verify(floating >= -2 && floating < 4);
   }
}

$test(random_lemire_bounded_reference) {
   cat::uint1 source = 0u;
   cat::idx calls = 0u;
   cat::array<cat::idx, 10u> counts{};
   auto next = [&] {
      cat::uint1 const value = source;
      ++source;
      ++calls;
      return value;
   };
   for (cat::idx draw = 0u; draw < 250u; ++draw) {
      ++counts[cat::idx(cat::detail::lemire_bounded(cat::uint1(10u), next))];
   }
   cat::verify(calls == 256u);
   for (cat::idx count : counts) {
      cat::verify(count == 25u);
   }

   cat::uint8 rejecting_source = 0u;
   calls = 0u;
   auto reject_once = [&] {
      cat::uint8 const value = rejecting_source;
      ++rejecting_source;
      ++calls;
      return value;
   };
   cat::verify(cat::detail::lemire_bounded(cat::uint8(10u), reject_once) == 0u);
   cat::verify(calls == 2u);

   auto maximum = [] {
      return cat::limits<cat::uint8>::max();
   };
   cat::verify(cat::detail::lemire_bounded(cat::uint8(10u), maximum) == 9u);

   auto upper_bits = [] {
      return cat::uint8(0xa0000000'00000000ull);
   };
   cat::verify(cat::detail::lemire_bounded(cat::uint8(8u), upper_bits) == 5u);
   cat::verify(cat::detail::lemire_bounded(cat::uint8(1u), upper_bits) == 0u);

   cat::array<cat::uint8x2, 2u> simd_words{
      cat::uint8x2{0u, cat::uint8::max()},
      cat::uint8x2{1u, 0u               },
   };
   calls = 0u;
   cat::uint8x2 const simd_result =
      cat::detail::lemire_bounded(cat::uint8x2{10u, 0u}, [&] {
         cat::uint8x2 const value = simd_words[calls];
         ++calls;
         return value;
      });
   cat::verify(simd_result == cat::uint8x2{0u, cat::uint8::max()});
   cat::verify(calls == 2u);
}

$test(random_non_full_range_integer_generation) {
   cat::independent_bits_engine<ternary_engine, 1u, cat::uint1> bits(
      ternary_engine{}
   );
   cat::idx zeros = 0u;
   cat::idx ones = 0u;
   for (cat::idx draw = 0u; draw < 8u; ++draw) {
      if (bits() == 0u) {
         ++zeros;
      } else {
         ++ones;
      }
   }
   cat::verify(zeros == 4u && ones == 4u);
   cat::verify(bits.base().calls() == 11u);

   simd_binary_engine<cat::uint4x4> engine;
   cat::uniform_int_distribution<cat::uint4x4> distribution(
      cat::uint4x4(0u), cat::uint4x4(1u)
   );
   cat::verify(distribution(engine) == cat::uint4x4(1u));
   cat::verify(engine.calls() == 32u);

   simd_binary_engine<cat::uint8x2> wide_engine;
   cat::uniform_int_distribution<cat::uint8x2> wide_distribution(
      cat::uint8x2(0u), cat::uint8x2(1u)
   );
   cat::verify(wide_distribution(wide_engine) == cat::uint8x2(1u));
   cat::verify(wide_engine.calls() == 64u);

   canonical_counting_engine<cat::uint4> full_range(0u);
   cat::uniform_int_distribution<cat::int4> signed_full(
      cat::limits<cat::int4>::min(), cat::limits<cat::int4>::max()
   );
   cat::verify(signed_full(full_range) == cat::limits<cat::int4>::min());
   cat::verify(full_range.calls() == 1u);

   canonical_counting_engine<cat::uint4> singleton(123u);
   cat::verify(
      cat::uniform_int_distribution<cat::int4>(-7, -7)(singleton) == -7
   );
   cat::verify(singleton.calls() == 1u);
}

$test(random_canonical_generation) {
   canonical_counting_engine<cat::uint8> wide(0x01234567'89abcdefull);
   cat::float8 const wide_value =
      cat::detail::distribution_generate_canonical<cat::float8>(wide);
   cat::verify(wide.calls() == 1u);
   cat::verify(wide_value >= 0 && wide_value < 1);

   canonical_counting_engine<cat::uint4> words(0x12345678u);
   cat::float8 const word_value =
      cat::detail::distribution_generate_canonical<cat::float8>(words);
   cat::verify(words.calls() == 2u);
   cat::verify(word_value >= 0 && word_value < 1);

   canonical_counting_engine<cat::uint8> maximum(
      cat::limits<cat::uint8>::max()
   );
   cat::float8 const maximum_value =
      cat::detail::distribution_generate_canonical<cat::float8>(maximum);
   cat::verify(maximum.calls() == 1u);
   cat::verify(maximum_value >= 0 && maximum_value < 1);

   canonical_bounded_engine<cat::uint4, 5u, 12u> power_two(5u);
   cat::verify(
      cat::detail::distribution_generate_canonical<cat::float8>(power_two) == 0
   );
   cat::verify(power_two.calls() == 18u);

   canonical_bounded_engine<cat::uint4, 10u, 15u> non_power(15u);
   cat::float8 const non_power_value =
      cat::detail::distribution_generate_canonical<cat::float8>(non_power);
   cat::verify(non_power.calls() == 21u);
   cat::verify(non_power_value >= 0 && non_power_value < 1);

   canonical_bounded_engine<cat::uint4, 4u, 5u> one_bit(4u);
   static_cast<void>(
      cat::detail::distribution_generate_canonical<cat::float8>(one_bit)
   );
   cat::verify(one_bit.calls() == 53u);

   canonical_decimal_engine decimal;
   cat::float4 const reference =
      cat::detail::distribution_generate_canonical<cat::float4>(decimal);
   cat::verify(decimal.calls() == 8u);
   cat::verify(reference == 0x1.87e6b6p-1f);

   canonical_decimal_engine public_decimal;
   cat::verify(
      cat::generate_canonical<cat::float4>(public_decimal) == reference
   );
   cat::verify(public_decimal.calls() == 8u);

   canonical_bounded_engine<cat::uint4, 4u, 5u> public_one_bit(4u);
   static_cast<void>(cat::generate_canonical<cat::float8>(public_one_bit));
   cat::verify(public_one_bit.calls() == 53u);

   canonical_bounded_engine<cat::uint4x4, 5u, 12u> simd_narrow(
      cat::uint4x4{5u, 6u, 11u, 12u}
   );
   cat::float4x4 const narrow_values =
      cat::detail::distribution_generate_canonical<cat::float4x4>(simd_narrow);
   cat::verify(simd_narrow.calls() == 8u);
   for (cat::float4 value : narrow_values) {
      cat::verify(value >= 0.f && value < 1.f);
   }

   cat::uint8x2 const simd_word{0x01234567'89abcdefull, 0xfedcba98'76543210ull};
   canonical_counting_engine<cat::uint8x2> simd_engine(simd_word);
   cat::float8x2 const simd_values =
      cat::detail::distribution_generate_canonical<cat::float8x2>(simd_engine);
   cat::verify(simd_engine.calls() == 1u);
   for (cat::idx lane = 0u; lane < 2u; ++lane) {
      canonical_counting_engine<cat::uint8> scalar_engine(simd_word[lane]);
      cat::verify(
         simd_values[lane]
         == cat::detail::distribution_generate_canonical<cat::float8>(
            scalar_engine
         )
      );
   }
}

$test(random_canonical_distribution_calls) {
   {
      canonical_counting_engine<cat::uint8> engine(1u);
      static_cast<void>(
         cat::uniform_float_distribution<cat::float8>(0, 1)(engine)
      );
      cat::verify(engine.calls() == 1u);
   }
   {
      canonical_counting_engine<cat::uint8> engine(1u);
      static_cast<void>(cat::bernoulli_distribution<cat::float8>(0.5)(engine));
      cat::verify(engine.calls() == 1u);
   }
   {
      canonical_counting_engine<cat::uint8> engine(1u);
      static_cast<void>(cat::cauchy_distribution<cat::float8>(0, 1)(engine));
      cat::verify(engine.calls() == 1u);
   }
   {
      canonical_counting_engine<cat::uint8> engine(0x80000000'00000000ull);
      cat::normal_distribution<cat::float8> distribution(0, 1);
      static_cast<void>(distribution(engine));
      cat::verify(engine.calls() == 2u);
      static_cast<void>(distribution(engine));
      cat::verify(engine.calls() == 2u);
   }
}

$test(random_distribution_families) {
   cat::xoshiro_engine<cat::uint8> engine(321u);
   cat::bernoulli_distribution<cat::float8_fast> fast_bernoulli(0.5);
   cat::binomial_distribution<cat::int4, cat::float8_fast> fast_binomial(
      10, 0.5
   );
   cat::geometric_distribution<cat::int4, cat::float8_fast> fast_geometric(0.5);
   cat::poisson_distribution<cat::int4, cat::float8_fast> fast_poisson(4);
   static_assert(cat::is_same<decltype(fast_bernoulli.p()), cat::float8_fast>);
   static_assert(cat::is_same<decltype(fast_binomial.p()), cat::float8_fast>);
   static_cast<void>(fast_bernoulli(engine));
   cat::verify(fast_binomial(engine) >= 0);
   cat::verify(fast_geometric(engine) >= 0);
   cat::verify(fast_poisson(engine) >= 0);

   cat::verify(cat::binomial_distribution<>(10, 0.5)(engine) >= 0);
   cat::verify(cat::negative_binomial_distribution<>(3, 0.5)(engine) >= 0);
   cat::verify(cat::geometric_distribution<>(0.5)(engine) >= 0);
   cat::verify(cat::poisson_distribution<>(4)(engine) >= 0);
   cat::verify(cat::poisson_distribution<>(40)(engine) >= 0);
   cat::verify(cat::exponential_distribution<>(2)(engine) >= 0);
   cat::verify(cat::gamma_distribution<>(2, 3)(engine) >= 0);
   cat::verify(cat::weibull_distribution<>(2, 3)(engine) >= 0);
   static_cast<void>(cat::extreme_value_distribution<>(0, 1)(engine));
   static_cast<void>(cat::normal_distribution<>(0, 1)(engine));
   cat::verify(cat::lognormal_distribution<>(0, 1)(engine) > 0);
   cat::verify(cat::chi_squared_distribution<>(2)(engine) >= 0);
   static_cast<void>(cat::cauchy_distribution<>(0, 1)(engine));
   cat::verify(cat::fisher_f_distribution<>(2, 3)(engine) >= 0);
   static_cast<void>(cat::student_t_distribution<>(3)(engine));

   cat::discrete_distribution<> discrete{1, 2, 3};
   cat::verify(discrete(engine) >= 0 && discrete(engine) <= 2);
   cat::piecewise_constant_distribution<> constant({0, 1, 2}, {1, 2});
   cat::float8 const constant_value = constant(engine);
   cat::verify(constant_value >= 0 && constant_value < 2);
   cat::piecewise_linear_distribution<> linear({0, 1, 2}, {1, 2, 1});
   cat::float8 const linear_value = linear(engine);
   cat::verify(linear_value >= 0 && linear_value < 2);
}

$test(random_distribution_bounds) {
   constexpr cat::float8 infinity = cat::limits<cat::float8>::infinity();

   static_assert(cat::normal_distribution<>{}.min() == -infinity);
   static_assert(cat::normal_distribution<>{}.max() == infinity);
   static_assert(cat::cauchy_distribution<>{}.min() == -infinity);
   static_assert(cat::cauchy_distribution<>{}.max() == infinity);
   static_assert(cat::student_t_distribution<>{}.min() == -infinity);
   static_assert(cat::student_t_distribution<>{}.max() == infinity);
   static_assert(cat::extreme_value_distribution<>{}.min() == -infinity);
   static_assert(cat::extreme_value_distribution<>{}.max() == infinity);
   static_assert(cat::exponential_distribution<>{}.max() == infinity);
   static_assert(cat::gamma_distribution<>{}.max() == infinity);
   static_assert(cat::chi_squared_distribution<>{}.max() == infinity);
   static_assert(cat::fisher_f_distribution<>{}.max() == infinity);
   static_assert(cat::weibull_distribution<>{}.max() == infinity);
   static_assert(cat::lognormal_distribution<>{}.max() == infinity);
   static_assert(cat::binomial_distribution<>(10, 0.5).max() == 10);

   using wide_float = cat::float8x2;
   wide_float const wide_infinity = cat::limits<wide_float>::infinity();
   cat::verify(cat::cauchy_distribution<wide_float>{}.min() == -wide_infinity);
   cat::verify(cat::cauchy_distribution<wide_float>{}.max() == wide_infinity);
}

$test(random_fill_scalar_forms) {
   static_assert(
      cat::is_same<
         cat::convenience_random_engine, cat::pcg_dxsm_engine<cat::uint8>>
   );

   cat::array<cat::uint8, 17u> direct;
   cat::array<cat::uint8, 17u> expected;
   cat::xoshiro_engine<cat::uint8> direct_engine(44u);
   cat::xoshiro_engine<cat::uint8> expected_engine(44u);
   cat::verify(
      cat::fill_random(direct, direct_engine) == cat::iteration_result::complete
   );
   cat::fill_random(expected, expected_engine);
   cat::verify(direct == expected);
   cat::verify(direct_engine() == expected_engine());

   cat::array<cat::int4, 17u> piped;
   cat::array<cat::int4, 17u> piped_expected;
   cat::xoshiro_engine<cat::uint8> piped_engine(55u);
   cat::xoshiro_engine<cat::uint8> piped_expected_engine(55u);
   cat::uniform_int_distribution<cat::int4> piped_distribution(-12, 19);
   cat::uniform_int_distribution<cat::int4> expected_distribution(-12, 19);
   auto&& pipe_result =
      piped | cat::fill_random(piped_engine, piped_distribution);
   cat::verify(__builtin_addressof(pipe_result) == __builtin_addressof(piped));
   cat::fill_random(
      piped_expected, piped_expected_engine, expected_distribution
   );
   cat::verify(piped == piped_expected);
   cat::verify(piped_engine() == piped_expected_engine());

   cat::array<cat::uint8, 17u> member;
   cat::array<cat::uint8, 17u> member_expected;
   cat::xoshiro_engine<cat::uint8> member_engine(66u);
   cat::xoshiro_engine<cat::uint8> member_expected_engine(66u);
   auto&& member_result = member.fill_random(member_engine);
   cat::verify(
      __builtin_addressof(member_result) == __builtin_addressof(member)
   );
   member_expected.fill_random(member_expected_engine);
   cat::verify(member == member_expected);
   cat::verify(member_engine() == member_expected_engine());

   cat::array<cat::uint8, 17u> pipe_engine_only;
   cat::array<cat::uint8, 17u> pipe_engine_only_expected;
   cat::xoshiro_engine<cat::uint8> pipe_engine(67u);
   cat::xoshiro_engine<cat::uint8> pipe_expected_engine(67u);
   pipe_engine_only | cat::fill_random(pipe_engine);
   cat::fill_random(pipe_engine_only_expected, pipe_expected_engine);
   cat::verify(pipe_engine_only == pipe_engine_only_expected);
   cat::verify(pipe_engine() == pipe_expected_engine());
}

$test(random_fill_scalar_engine_awkward_tails) {
   cat::array<cat::uint8, 37u> values;
   cat::array<cat::uint8, 37u> expected;
   cat::pcg_dxsm_engine<cat::uint8> engine(123u, 456u);
   cat::pcg_dxsm_engine<cat::uint8> expected_engine(123u, 456u);
   cat::fill_random(values, engine);
   for (cat::uint8& value : expected) {
      value = expected_engine();
   }
   cat::verify(values == expected);
   cat::verify(engine() == expected_engine());
}

$test(random_internal_distribution_batch_kernels) {
   using abi_type = cat::simd_abi::fixed_size<cat::uint8, 2u>;

   cat::pcg_dxsm_engine<cat::uint8> engine(321u, 654u);
   auto const generate = [&engine](auto distribution) {
      return cat::detail::generate_random_distribution_batch<abi_type>(
         distribution, engine
      );
   };
   static_assert(!has_public_random_batch<
                 cat::bernoulli_distribution<cat::float8>, abi_type>);

   auto bernoulli = generate(cat::bernoulli_distribution<cat::float8>(0.25));
   static_assert(cat::is_simd_mask<decltype(bernoulli)>);

   auto binomial = generate(cat::binomial_distribution<>(7, 0.4));
   auto cauchy = generate(cat::cauchy_distribution<>(1, 2));
   auto chi = generate(cat::chi_squared_distribution<>(3));
   auto discrete = generate(cat::discrete_distribution<>({1, 2, 3}));
   auto exponential = generate(cat::exponential_distribution<>(2));
   auto extreme = generate(cat::extreme_value_distribution<>(1, 2));
   auto fisher = generate(cat::fisher_f_distribution<>(2, 3));
   auto gamma = generate(cat::gamma_distribution<>(2, 3));
   auto geometric = generate(cat::geometric_distribution<>(0.4));
   auto lognormal = generate(cat::lognormal_distribution<>(0, 1));
   auto negative = generate(cat::negative_binomial_distribution<>(3, 0.4));
   auto normal = generate(cat::normal_distribution<>(0, 1));
   auto piecewise_constant =
      generate(cat::piecewise_constant_distribution<>({0, 1, 2}, {1, 2}));
   auto piecewise_linear =
      generate(cat::piecewise_linear_distribution<>({0, 1, 2}, {1, 2, 1}));
   auto poisson = generate(cat::poisson_distribution<>(4));
   auto student = generate(cat::student_t_distribution<>(3));
   auto uniform_float = generate(cat::uniform_float_distribution<>(-2, 4));
   auto uniform_int = generate(cat::uniform_int_distribution<>(-8, 13));
   auto weibull = generate(cat::weibull_distribution<>(2, 3));

   static_assert(cat::is_simd<decltype(binomial)>);
   static_assert(decltype(binomial)::abi_type::lanes == 2u);
   for (cat::idx lane = 0u; lane < 2u; ++lane) {
      cat::verify(binomial[lane] >= 0 && binomial[lane] <= 7);
      cat::verify(chi[lane] >= 0);
      cat::verify(discrete[lane] >= 0 && discrete[lane] <= 2);
      cat::verify(exponential[lane] >= 0);
      cat::verify(fisher[lane] >= 0);
      cat::verify(gamma[lane] >= 0);
      cat::verify(geometric[lane] >= 0);
      cat::verify(lognormal[lane] > 0);
      cat::verify(negative[lane] >= 0);
      cat::verify(
         piecewise_constant[lane] >= 0 && piecewise_constant[lane] < 2
      );
      cat::verify(piecewise_linear[lane] >= 0 && piecewise_linear[lane] < 2);
      cat::verify(poisson[lane] >= 0);
      cat::verify(uniform_float[lane] >= -2 && uniform_float[lane] < 4);
      cat::verify(uniform_int[lane] >= -8 && uniform_int[lane] <= 13);
      cat::verify(weibull[lane] >= 0);
      static_cast<void>(cauchy[lane]);
      static_cast<void>(extreme[lane]);
      static_cast<void>(normal[lane]);
      static_cast<void>(student[lane]);
   }
}

$test(random_distribution_cache_continuity) {
   cat::pcg_dxsm_engine<cat::uint8> left_engine(999u, 111u);
   cat::pcg_dxsm_engine<cat::uint8> right_engine(999u, 111u);
   cat::normal_distribution<> left(3, 2);
   cat::normal_distribution<> right(3, 2);
   cat::verify(left(left_engine) == right(right_engine));
   cat::array<cat::float8, 11u> left_normal;
   cat::array<cat::float8, 11u> right_normal;
   cat::fill_random(left_normal, left_engine, left);
   for (cat::float8& value : right_normal) {
      value = right(right_engine);
   }
   for (cat::idx index = 0u; index < left_normal.size(); ++index) {
      cat::float8 const difference =
         left_normal[index] < right_normal[index]
            ? right_normal[index] - left_normal[index]
            : left_normal[index] - right_normal[index];
      cat::float8 const magnitude =
         right_normal[index] < 0.f ? -right_normal[index] : right_normal[index];
      cat::verify(difference <= 0x1p-40 * (1.f + magnitude));
   }
   cat::verify(left == right);
   cat::verify(left(left_engine) == right(right_engine));
   cat::verify(left_engine() == right_engine());

   cat::pcg_dxsm_engine<cat::uint8> left_log_engine(222u, 333u);
   cat::pcg_dxsm_engine<cat::uint8> right_log_engine(222u, 333u);
   cat::lognormal_distribution<> left_log(1, 2);
   cat::lognormal_distribution<> right_log(1, 2);
   cat::verify(left_log(left_log_engine) == right_log(right_log_engine));
   cat::array<cat::float8, 11u> left_lognormal;
   cat::array<cat::float8, 11u> right_lognormal;
   cat::fill_random(left_lognormal, left_log_engine, left_log);
   for (cat::float8& value : right_lognormal) {
      value = right_log(right_log_engine);
   }
   for (cat::idx index = 0u; index < left_lognormal.size(); ++index) {
      cat::float8 const difference =
         left_lognormal[index] < right_lognormal[index]
            ? right_lognormal[index] - left_lognormal[index]
            : left_lognormal[index] - right_lognormal[index];
      cat::verify(difference <= 0x1p-40 * (1.f + right_lognormal[index]));
   }
   cat::verify(left_log == right_log);
   cat::float8 const left_log_next = left_log(left_log_engine);
   cat::float8 const right_log_next = right_log(right_log_engine);
   cat::float8 const next_difference = left_log_next < right_log_next
                                          ? right_log_next - left_log_next
                                          : left_log_next - right_log_next;
   cat::verify(next_difference <= 0x1p-40 * (1.f + right_log_next));
   cat::verify(left_log_engine() == right_log_engine());
}

$test(random_fill_customization_hooks) {
   cat::array<cat::uint4, 5u> values;
   random_fill_custom_engine engine;
   cat::fill_random(values, engine);
   cat::verify(engine.used);
   for (cat::uint4 value : values) {
      cat::verify(value == 42u);
   }

   random_fill_custom_distribution distribution;
   cat::fill_random(values, engine, distribution);
   cat::verify(distribution.used);
   for (cat::uint4 value : values) {
      cat::verify(value == 17u);
   }
}

$test(random_fill_simd_parity_and_tails) {
   cat::array<cat::uint4, 37u> values;
   cat::array<cat::uint4, 37u> expected;
   cat::xoshiro_engine<cat::uint4x4> engine(77u);
   cat::xoshiro_engine<cat::uint4x4> expected_engine(77u);
   cat::fill_random(values, engine);

   cat::uint4x4 expected_values;
   cat::idx expected_lane = cat::uint4x4::abi_type::lanes;
   for (cat::uint4& value : expected) {
      if (expected_lane == cat::uint4x4::abi_type::lanes) {
         expected_values = expected_engine();
         expected_lane = 0u;
      }
      value = expected_values[expected_lane];
      ++expected_lane;
   }
   cat::verify(values == expected);
   cat::verify(engine() == expected_engine());

   cat::array<cat::int4, 19u> distributed;
   cat::array<cat::int4, 19u> distributed_expected;
   cat::xoshiro_engine<cat::uint4x4> distribution_engine(88u);
   cat::xoshiro_engine<cat::uint4x4> expected_distribution_engine(88u);
   cat::uniform_int_distribution<cat::int4x4> distribution(
      cat::int4x4(-20), cat::int4x4(30)
   );
   cat::uniform_int_distribution<cat::int4x4> distribution_expected(
      cat::int4x4(-20), cat::int4x4(30)
   );
   distributed.fill_random(distribution_engine, distribution);

   cat::int4x4 expected_distributed_values;
   cat::idx distributed_lane = cat::int4x4::abi_type::lanes;
   for (cat::int4& value : distributed_expected) {
      if (distributed_lane == cat::int4x4::abi_type::lanes) {
         expected_distributed_values =
            distribution_expected(expected_distribution_engine);
         distributed_lane = 0u;
      }
      value = expected_distributed_values[distributed_lane];
      ++distributed_lane;
   }
   cat::verify(distributed == distributed_expected);
   cat::verify(distribution_engine() == expected_distribution_engine());

   cat::array<bool, 9u> booleans;
   cat::array<bool, 9u> booleans_expected;
   cat::xoshiro_engine<cat::uint8x2> boolean_engine(89u);
   cat::xoshiro_engine<cat::uint8x2> expected_boolean_engine(89u);
   cat::bernoulli_distribution<cat::float8x2> boolean_distribution(
      cat::float8x2(0.375)
   );
   cat::bernoulli_distribution<cat::float8x2> expected_boolean_distribution(
      cat::float8x2(0.375)
   );
   cat::fill_random(booleans, boolean_engine, boolean_distribution);

   cat::float8x2::mask_type expected_boolean_values;
   cat::idx boolean_lane = cat::float8x2::abi_type::lanes;
   for (bool& value : booleans_expected) {
      if (boolean_lane == cat::float8x2::abi_type::lanes) {
         expected_boolean_values =
            expected_boolean_distribution(expected_boolean_engine);
         boolean_lane = 0u;
      }
      value = expected_boolean_values[boolean_lane];
      ++boolean_lane;
   }
   cat::verify(booleans == booleans_expected);
   cat::verify(boolean_engine() == expected_boolean_engine());
}

template <typename Abi>
void
verify_xoshiro_relaxed_bulk_tails() {
   using simd_type = cat::simd<cat::uint8, Abi>;
   constexpr cat::idx capacity = (Abi::lanes * 2u) + 1u;
   for (cat::idx size = 0u; size <= capacity; ++size) {
      cat::array<cat::uint8, capacity> left;
      cat::array<cat::uint8, capacity> right;
      left.fill(0xfeedfaceu);
      right.fill(0xfeedfaceu);
      cat::xoshiro_pp_engine<cat::uint8> left_engine(707u);
      cat::xoshiro_pp_engine<cat::uint8> right_engine(707u);
      cat::detail::fill_random_relaxed_bulk_contiguous<simd_type>(
         left.data(), size, left_engine
      );
      cat::detail::fill_random_relaxed_bulk_contiguous<simd_type>(
         right.data(), size, right_engine
      );
      cat::verify(left == right);
      cat::verify(left_engine() == right_engine());
      for (cat::idx index = size; index < capacity; ++index) {
         cat::verify(left[index] == 0xfeedfaceu);
      }
   }
}

[[gnu::target("avx512f,avx512cd,avx512bw,avx512dq,avx512vl")]]
void
verify_xoshiro_relaxed_bulk_avx512_tails() {
   verify_xoshiro_relaxed_bulk_tails<x64::avx512_abi<cat::uint8>>();
}

template <cat::idx size, typename Distribution>
void
verify_xoshiro_distribution_tail(Distribution distribution) {
   using result_type = Distribution::result_type;
   cat::array<result_type, size> left;
   cat::array<result_type, size> right;
   cat::xoshiro_pp_engine<cat::uint8> left_engine(808u);
   cat::xoshiro_pp_engine<cat::uint8> right_engine(808u);
   Distribution right_distribution = distribution;
   cat::fill_random(left, left_engine, distribution);
   cat::fill_random(right, right_engine, right_distribution);
   cat::verify(left == right);
   cat::verify(distribution == right_distribution);
   cat::verify(left_engine() == right_engine());
}

template <cat::idx size, typename Distribution>
void
verify_xoshiro_relaxed_distribution_tail(Distribution distribution) {
   using abi_type = x64::avx_abi<cat::uint8>;
   using result_type = Distribution::result_type;
   cat::array<result_type, size> left;
   cat::array<result_type, size> right;
   cat::xoshiro_pp_engine<cat::uint8> left_engine(909u);
   cat::xoshiro_pp_engine<cat::uint8> right_engine(909u);
   Distribution right_distribution = distribution;

   using session_type =
      cat::remove_cvref<decltype(cat::detail::make_relaxed_random_bulk_session<
                                 abi_type>(right_engine))>;
   using batch_type = cat::remove_cvref<
      decltype(cat::detail::generate_random_distribution_batch<abi_type>(
         right_distribution, cat::declval<session_type&>()
      ))>;

   cat::detail::fill_random_relaxed_distribution_contiguous<
      batch_type, result_type, decltype(left_engine), Distribution, abi_type>(
      left.data(), size, left_engine, distribution
   );

   auto session =
      cat::detail::make_relaxed_random_bulk_session<abi_type>(right_engine);
   cat::detail::fill_random_distribution_batch_contiguous<batch_type>(
      right.data(), size, [&] {
         return cat::detail::generate_random_distribution_batch<abi_type>(
            right_distribution, session
         );
      }
   );
   right_engine.discard(session.scalar_draws());

   cat::verify(left == right);
   cat::verify(distribution == right_distribution);
   cat::verify(left_engine() == right_engine());
}

$test(random_fill_xoshiro_relaxed_bulk_accounting) {
   static_assert(cat::xoshiro_pp_engine<cat::uint8>::enable_relaxed_bulk_fill);
   static_assert(
      cat::xoroshiro_pp_engine<cat::uint8>::enable_relaxed_bulk_fill
   );
   static_assert(
      cat::xoshiro512_pp_engine<cat::uint8>::enable_relaxed_bulk_fill
   );
   static_assert(
      !cat::xoroshiro1024_pp_engine<cat::uint8>::enable_relaxed_bulk_fill
   );
   static_assert(
      cat::detail::relaxed_random_bulk_threshold<
         x64::sse_abi<cat::uint8>, cat::xoshiro_pp_engine<cat::uint8>>()
      == 16u
   );
   static_assert(
      cat::detail::relaxed_random_bulk_threshold<
         x64::avx_abi<cat::uint8>, cat::xoshiro_pp_engine<cat::uint8>>()
      == 16u
   );
   static_assert(
      cat::detail::relaxed_random_bulk_threshold<
         x64::avx512_abi<cat::uint8>, cat::xoshiro_pp_engine<cat::uint8>>()
      == cat::idx::max()
   );

   cat::array<cat::uint8, 1'025u> left;
   cat::array<cat::uint8, 1'025u> right;
   cat::xoshiro_pp_engine<cat::uint8> left_engine(707u);
   cat::xoshiro_pp_engine<cat::uint8> right_engine(707u);
   cat::xoshiro_pp_engine<cat::uint8> continuation(707u);
   cat::fill_random(left, left_engine);
   cat::fill_random(right, right_engine);
   continuation.discard(left.size());
   cat::verify(left == right);
   cat::uint8 const left_next = left_engine();
   cat::uint8 const right_next = right_engine();
   cat::uint8 const continuation_next = continuation();
   cat::verify(left_next == right_next);
   cat::verify(left_next == continuation_next);

   cat::array<cat::uint8, 15u> below_threshold;
   cat::xoshiro_pp_engine<cat::uint8> below_engine(707u);
   cat::xoshiro_pp_engine<cat::uint8> below_scalar(707u);
   cat::fill_random(below_threshold, below_engine);
   for (cat::uint8 value : below_threshold) {
      cat::verify(value == below_scalar());
   }
   cat::verify(below_engine() == below_scalar());

   verify_xoshiro_relaxed_bulk_tails<x64::sse_abi<cat::uint8>>();
   verify_xoshiro_relaxed_bulk_tails<x64::avx_abi<cat::uint8>>();
   if (
      __builtin_cpu_supports("avx512f") && __builtin_cpu_supports("avx512cd")
      && __builtin_cpu_supports("avx512bw")
      && __builtin_cpu_supports("avx512dq")
      && __builtin_cpu_supports("avx512vl")
   ) {
      verify_xoshiro_relaxed_bulk_avx512_tails();
   }
}

$test(random_fill_xoshiro_relaxed_distributions) {
   cat::array<cat::int4, 1'025u> left_integers;
   cat::array<cat::int4, 1'025u> right_integers;
   cat::xoshiro_pp_engine<cat::uint8> left_engine(808u);
   cat::xoshiro_pp_engine<cat::uint8> right_engine(808u);
   cat::uniform_int_distribution<cat::int4> left_uniform(-11, 23);
   cat::uniform_int_distribution<cat::int4> right_uniform(-11, 23);
   cat::fill_random(left_integers, left_engine, left_uniform);
   cat::fill_random(right_integers, right_engine, right_uniform);
   cat::verify(left_integers == right_integers);
   cat::verify(left_engine() == right_engine());
   for (cat::int4 value : left_integers) {
      cat::verify(value >= -11 && value <= 23);
   }

   cat::array<cat::float8, 1'025u> left_normals;
   cat::array<cat::float8, 1'025u> right_normals;
   cat::normal_distribution<cat::float8> left_normal(2, 3);
   cat::normal_distribution<cat::float8> right_normal(2, 3);
   cat::fill_random(left_normals, left_engine, left_normal);
   cat::fill_random(right_normals, right_engine, right_normal);
   cat::verify(left_normals == right_normals);
   cat::verify(left_normal == right_normal);
   cat::verify(left_engine() == right_engine());

   cat::array<bool, 1'025u> left_booleans;
   cat::array<bool, 1'025u> right_booleans;
   cat::bernoulli_distribution<> left_bernoulli(0.375);
   cat::bernoulli_distribution<> right_bernoulli(0.375);
   cat::fill_random(left_booleans, left_engine, left_bernoulli);
   cat::fill_random(right_booleans, right_engine, right_bernoulli);
   cat::verify(left_booleans == right_booleans);
   cat::verify(left_engine() == right_engine());

   verify_xoshiro_distribution_tail<255u>(
      cat::uniform_int_distribution<cat::int4>(-11, 23)
   );
   verify_xoshiro_distribution_tail<256u>(
      cat::uniform_float_distribution<cat::float8>(-2, 5)
   );
   verify_xoshiro_distribution_tail<257u>(
      cat::normal_distribution<cat::float8>(2, 3)
   );
   verify_xoshiro_distribution_tail<259u>(
      cat::gamma_distribution<cat::float8>(2, 3)
   );
   verify_xoshiro_distribution_tail<263u>(
      cat::poisson_distribution<cat::int4>(4)
   );

   verify_xoshiro_relaxed_distribution_tail<255u>(
      cat::uniform_int_distribution<cat::int4>(-11, 23)
   );
   verify_xoshiro_relaxed_distribution_tail<256u>(
      cat::uniform_float_distribution<cat::float8>(-2, 5)
   );
   verify_xoshiro_relaxed_distribution_tail<257u>(
      cat::bernoulli_distribution<cat::float8>(0.375)
   );
   verify_xoshiro_relaxed_distribution_tail<259u>(
      cat::normal_distribution<cat::float8>(2, 3)
   );
   verify_xoshiro_relaxed_distribution_tail<263u>(
      cat::gamma_distribution<cat::float8>(2, 3)
   );
   verify_xoshiro_relaxed_distribution_tail<269u>(
      cat::poisson_distribution<cat::int4>(4)
   );
}

$test(random_fill_factories) {
   cat::xoshiro_engine<cat::uint8> array_engine(99u);
   auto fixed =
      cat::make_filled_random<cat::array<cat::uint8, 13u>>(array_engine);
   cat::verify(fixed.size() == 13u);

   cat::xoshiro_engine<cat::uint8> fixed_distribution_engine(100u);
   cat::uniform_int_distribution<cat::int4> fixed_distribution(-8, 12);
   auto fixed_distributed = cat::make_filled_random<cat::array<cat::int4, 7u>>(
      fixed_distribution_engine, fixed_distribution
   );
   for (cat::int4 value : fixed_distributed) {
      cat::verify(value >= -8 && value <= 12);
   }

   cat::xoshiro_engine<cat::uint8> inplace_engine(100u);
   auto inplace = cat::make_filled_random<cat::vec_inplace<cat::uint8, 32u>>(
                     21u, inplace_engine
   )
                     .verify();
   cat::verify(inplace.size() == 21u);

   cat::xoshiro_engine<cat::uint8> distributed_engine(101u);
   cat::uniform_int_distribution<cat::int4> distribution(-4, 7);
   auto distributed = cat::make_filled_random<cat::vec_inplace<cat::int4, 16u>>(
                         11u, distributed_engine, distribution
   )
                         .verify();
   cat::verify(distributed.size() == 11u);
   for (cat::int4 value : distributed) {
      cat::verify(value >= -4 && value <= 7);
   }

   cat::xoshiro_engine<cat::uint8> manual_engine(102u);
   auto manual =
      cat::make_filled_random<cat::vec<cat::uint8>>(
         cat::allocator_ref<cat::page_allocator>(pager), 19u, manual_engine
      )
         .verify();
   cat::verify(manual.size() == 19u);
   manual.free(pager);

   using owned_vec = cat::raii::basic_vec<cat::uint8, cat::page_allocator>;
   cat::xoshiro_engine<cat::uint8> owned_engine(103u);
   auto owned =
      cat::raii::make_filled_random<owned_vec>(
         cat::allocator_ref<cat::page_allocator>(pager), 23u, owned_engine
      )
         .verify();
   cat::verify(owned.size() == 23u);

   cat::xoshiro_engine<cat::uint8> owned_distribution_engine(104u);
   cat::uniform_int_distribution<cat::uint8> owned_distribution(3u, 19u);
   auto distributed_owned =
      cat::raii::make_filled_random<owned_vec>(
         cat::allocator_ref<cat::page_allocator>(pager), 17u,
         owned_distribution_engine, owned_distribution
      )
         .verify();
   cat::verify(distributed_owned.size() == 17u);
}

$test(random_convenience) {
   cat::int4 const scalar = cat::random<cat::int4>(2, 9);
   cat::verify(scalar >= 2 && scalar <= 9);
   cat::int4x4 const vector =
      cat::random<cat::int4x4>(cat::int4x4(2), cat::int4x4(9));
   for (auto value : vector) {
      cat::verify(value >= 2 && value <= 9);
   }
}

$test(random_linux_sources) {
   static_assert(cat::is_uniform_random_bit_generator<nix::sys_urandom_engine>);
   static_assert(cat::is_uniform_random_bit_generator<nix::sys_random_engine>);
   static_assert(cat::is_uniform_random_bit_generator<nix::dev_urandom_engine>);
   static_assert(cat::is_uniform_random_bit_generator<nix::dev_random_engine>);

   nix::sys_urandom_engine urandom;
   cat::uint8 const first = urandom();
   cat::uint8 const second = urandom();
   cat::verify(first != second || urandom() != first);

   nix::sys_random_engine random;
   cat::uint8 const blocking_first = random();
   cat::uint8 const blocking_second = random();
   cat::verify(blocking_first != blocking_second || random() != blocking_first);

   nix::dev_urandom_engine dev_urandom;
   cat::uint8 const file_first = dev_urandom();
   cat::uint8 const file_second = dev_urandom();
   cat::verify(file_first != file_second || dev_urandom() != file_first);

   nix::dev_random_engine dev_random;
   cat::uint8 const file_blocking_first = dev_random();
   cat::uint8 const file_blocking_second = dev_random();
   cat::verify(
      file_blocking_first != file_blocking_second
      || dev_random() != file_blocking_first
   );

   cat::verify(x64::read_timestamp_counter() != 0u);
   auto const state = nix::make_seed_state();
   cat::verify(state.kernel_random != 0u || urandom() != 0u);
}
