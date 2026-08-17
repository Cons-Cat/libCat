#include <cat/random>

#include "../unit_tests.hpp"
#include "cat/debug"

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

   cat::wyrand_engine wy(0u);
   cat::verify(wy() == 0x111cb3a7'8f59a58eull);
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

   cat::wyrand_engine wy_restored(0u);
   wy_restored.discard(8u);
   wy_restored.backstep(8u);
   cat::verify(wy_restored() == cat::wyrand_engine(0u)());

   cat::discard_block_engine<cat::wyrand_engine, 5u, 3u> blocked(9u);
   cat::independent_bits_engine<cat::wyrand_engine, 16u, cat::uint4> bits(9u);
   cat::shuffle_order_engine<cat::wyrand_engine, 16u> shuffled(9u);
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

$test(random_bulk_and_convenience) {
   cat::array<cat::uint8, 16u> values;
   cat::xoshiro_engine<cat::uint8> engine(44u);
   cat::generate_random(values, engine);

   bool differs = false;
   for (cat::idx index = 1u; index < values.size(); ++index) {
      differs |= values[index] != values[0u];
   }
   cat::verify(differs);

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
