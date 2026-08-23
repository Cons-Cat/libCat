#include <cat/random>
#include <cat/splitmix>

#include "../unit_tests.hpp"

namespace {

using reference_word = __UINT64_TYPE__;

constexpr reference_word reference_increment = 0x9e3779b9'7f4a7c15ull;

constexpr auto
reference_next(reference_word& state) -> reference_word {
   state += reference_increment;
   reference_word value = state;
   value = (value ^ (value >> 30u)) * 0xbf58476d'1ce4e5b9ull;
   value = (value ^ (value >> 27u)) * 0x94d049bb'133111ebull;
   return value ^ (value >> 31u);
}

constexpr cat::array<cat::uint8, 8u> seed_zero_reference = {
   0xe220a839'7b1dcdafull, 0x6e789e6a'a1b965f4ull, 0x06c45d18'8009454full,
   0xf88bb8a8'724c81ecull, 0x1b39896a'51a8749bull, 0x53cb9f0c'747ea2eaull,
   0x2c829abe'1f4532e1ull, 0xc584133a'c916ab3cull,
};

constexpr cat::array<cat::uint8, 8u> seed_one_reference = {
   0x910a2dec'89025cc1ull, 0xbeeb8da1'658eec67ull, 0xf893a2ee'fb32555eull,
   0x71c18690'ee42c90bull, 0x71bb54d8'd101b5b9ull, 0xc34d0bff'90150280ull,
   0xe099ec6c'd7363ca5ull, 0x85e7bb0f'12278575ull,
};

constexpr cat::array<cat::uint8, 16u> variable_gamma_bug_seeds = {
   0x61c88646'80b583ebull, 0x7957d809'e827ff4cull, 0x305cb877'109d0686ull,
   0xefee3e7b'93db3075ull, 0xf8364607'e9c949bdull, 0xf8d059ae'e4c53639ull,
   0x359e58ee'afebd527ull, 0x79629ee7'6aa83059ull, 0x88e48f4f'cc823718ull,
   0x9cd9f015'db4e58b7ull, 0xbeb721c5'11b0da6dull, 0x05d507d0'5e785673ull,
   0x7f83ab8d'a2e71dd1ull, 0xf4077b0d'bebc73c0ull, 0x86466fd0'fcc363a6ull,
   0x76442b62'dddf926cull,
};

template <cat::idx size>
constexpr auto
matches_reference(cat::uint8 seed, cat::array<cat::uint8, size> const& expected)
   -> bool {
   cat::splitmix64_engine engine(seed);
   for (cat::idx index = 0u; index < size; ++index) {
      if (engine() != expected[index]) {
         return false;
      }
   }
   return true;
}

consteval auto
verify_constexpr_navigation() -> bool {
   cat::splitmix64_engine original(0u);
   cat::splitmix64_engine navigated(0u);
   navigated.jump(257u);
   navigated.backstep(257u);
   if (navigated != original) {
      return false;
   }
   navigated.set_state(0x12345678'90abcdefull);
   if (navigated.state() != 0x12345678'90abcdefull) {
      return false;
   }
   reference_word reference_state = 0x12345678'90abcdefull;
   return navigated() == reference_next(reference_state);
}

consteval auto
verify_constexpr_simd() -> bool {
   cat::splitmix_engine<cat::uint8x2> engine(0u);
   auto const values = engine();
   reference_word lane_0 = 0u;
   reference_word lane_1 = 1u;
   return values[0u] == reference_next(lane_0)
          && values[1u] == reference_next(lane_1);
}

consteval auto
verify_constexpr_batch() -> bool {
   using abi_type = cat::simd_abi::fixed_size<cat::uint8, 4u>;
   cat::splitmix64_engine batched(1u);
   cat::splitmix64_engine scalar(1u);
   auto const values =
      cat::detail::generate_exact_random_batch<abi_type>(batched);
   for (cat::idx lane = 0u; lane < 4u; ++lane) {
      if (values[lane] != scalar()) {
         return false;
      }
   }
   return batched == scalar;
}

template <typename Simd>
void
verify_simd_stream(cat::uint8 seed) {
   constexpr cat::idx lanes = Simd::abi_type::lanes;
   cat::splitmix_engine<Simd> wide(seed);
   cat::array<cat::splitmix64_engine, lanes> scalar;
   for (cat::idx lane = 0u; lane < lanes; ++lane) {
      scalar[lane].seed(seed + lane);
      cat::verify(wide.state()[lane] == seed + lane);
      for (cat::idx other = 0u; other < lane; ++other) {
         cat::verify(wide.state()[lane] != wide.state()[other]);
      }
   }

   for (cat::idx draw = 0u; draw < 16u; ++draw) {
      Simd const values = wide();
      for (cat::idx lane = 0u; lane < lanes; ++lane) {
         cat::verify(values[lane] == scalar[lane]());
      }
   }

   Simd const saved_state = wide.state();
   cat::splitmix_engine<Simd> restored(0u);
   restored.set_state(saved_state);
   cat::verify(restored == wide);

   Simd const counts = cat::iota<Simd>(3u);
   restored.discard(counts);
   for (cat::idx lane = 0u; lane < lanes; ++lane) {
      scalar[lane].discard(counts[lane]);
   }
   Simd const discarded = restored();
   for (cat::idx lane = 0u; lane < lanes; ++lane) {
      cat::verify(discarded[lane] == scalar[lane]());
   }
   restored.backstep(counts + 1u);
   cat::verify(restored == wide);

   cat::splitmix_engine<Simd> jumped;
   cat::splitmix_engine<Simd> discarded_by_lane;
   jumped.set_state(saved_state);
   discarded_by_lane.set_state(saved_state);
   jumped.jump(counts);
   discarded_by_lane.discard(counts);
   cat::verify(jumped == discarded_by_lane);

   jumped.set_state(saved_state);
   discarded_by_lane.set_state(saved_state);
   jumped.jump_log2(9u);
   discarded_by_lane.discard(512u);
   cat::verify(jumped == discarded_by_lane);

   jumped.set_state(saved_state);
   jumped.jump_log2(64u);
   cat::verify(jumped == wide);
   jumped.jump(Simd(cat::uint8::max()));
   jumped.jump(Simd(1u));
   cat::verify(jumped == wide);

   Simd const equal_state(17u);
   cat::splitmix_engine<Simd> explicitly_equal;
   explicitly_equal.set_state(equal_state);
   Simd const equal_values = explicitly_equal();
   for (cat::idx lane = 1u; lane < lanes; ++lane) {
      cat::verify(equal_values[lane] == equal_values[0u]);
   }
}

template <cat::idx lanes>
void
verify_scalar_batch(cat::uint8 seed) {
   using abi_type = cat::simd_abi::fixed_size<cat::uint8, lanes>;
   cat::splitmix64_engine batched(seed);
   cat::splitmix64_engine scalar(seed);
   batched.jump_log2(7u);
   scalar.discard(128u);
   auto const values =
      cat::detail::generate_exact_random_batch<abi_type>(batched);
   for (cat::idx lane = 0u; lane < lanes; ++lane) {
      cat::verify(values[lane] == scalar());
   }
   cat::verify(batched.state() == scalar.state());
   for (cat::idx draw = 0u; draw < 8u; ++draw) {
      cat::verify(batched() == scalar());
   }
}

template <cat::idx size>
void
verify_fill_random(cat::uint8 seed) {
   cat::array<cat::uint8, size> values;
   cat::splitmix64_engine filled(seed);
   cat::splitmix64_engine scalar(seed);
   cat::fill_random(values, filled);
   for (cat::idx index = 0u; index < size; ++index) {
      cat::verify(values[index] == scalar());
   }
   cat::verify(filled() == scalar());
}

static_assert(sizeof(reference_word) == 8u);
static_assert(
   cat::is_same<cat::splitmix64_engine, cat::splitmix_engine<cat::uint8>>
);
static_assert(cat::is_uniform_random_bit_generator<cat::splitmix64_engine>);
static_assert(
   cat::is_uniform_random_bit_generator<cat::splitmix_engine<cat::uint8x4>>
);
static_assert(matches_reference(0u, seed_zero_reference));
static_assert(matches_reference(1u, seed_one_reference));
static_assert(verify_constexpr_navigation());
static_assert(verify_constexpr_simd());
static_assert(verify_constexpr_batch());

}  // namespace

$test(splitmix_published_reference_streams) {
   cat::splitmix64_engine zero(0u);
   cat::splitmix64_engine one(1u);
   for (cat::idx index = 0u; index < seed_zero_reference.size(); ++index) {
      cat::verify(zero() == seed_zero_reference[index]);
      cat::verify(one() == seed_one_reference[index]);
   }
}

$test(splitmix_independent_reference_model) {
   for (cat::uint8 seed : variable_gamma_bug_seeds) {
      cat::splitmix64_engine engine(seed);
      reference_word state = seed.raw;
      for (cat::idx draw = 0u; draw < 32u; ++draw) {
         cat::verify(engine() == reference_next(state));
      }
   }

   cat::splitmix64_engine zero_output(0x61c88646'80b583ebull);
   cat::verify(zero_output() == 0u);
   cat::splitmix64_engine patterned_output(0x7957d809'e827ff4cull);
   cat::verify(patterned_output() == 0xaaaaaaaa'aaaaaaacull);
}

$test(splitmix_scalar_state_navigation) {
   cat::splitmix64_engine original(0xffffffff'ffffffffull);
   cat::splitmix64_engine iterated = original;
   cat::splitmix64_engine discarded = original;
   for (cat::idx index = 0u; index < 257u; ++index) {
      static_cast<void>(iterated());
   }
   discarded.discard(257u);
   cat::verify(discarded == iterated);
   discarded.backstep(257u);
   cat::verify(discarded == original);

   cat::splitmix64_engine jumped = original;
   discarded = original;
   jumped.jump(0x12345678'90abcdefull);
   discarded.discard(0x12345678'90abcdefull);
   cat::verify(jumped == discarded);
   jumped.backstep(0x12345678'90abcdefull);
   cat::verify(jumped == original);

   constexpr cat::array<cat::uword, 5u> exponents = {0u, 1u, 17u, 32u, 63u};
   for (cat::uword exponent : exponents) {
      jumped = original;
      discarded = original;
      jumped.jump_log2(exponent);
      discarded.discard(cat::uint8(1u).shift_left(exponent));
      cat::verify(jumped == discarded);
   }
   jumped = original;
   jumped.jump_log2(64u);
   cat::verify(jumped == original);
   jumped.jump_log2(127u);
   cat::verify(jumped == original);

   cat::splitmix64_engine restored;
   restored.set_state(iterated.state());
   cat::verify(restored == iterated);
   restored.discard(0xffffffff'ffffffffull);
   restored.discard(1u);
   cat::verify(restored == iterated);
   cat::verify(cat::splitmix64_engine::period_pow2() == 64u);
}

$test(splitmix_simd_lane_streams) {
   verify_simd_stream<cat::uint8x2>(0u);
   verify_simd_stream<cat::uint8x2>(0x00000001'00000000ull);
   verify_simd_stream<cat::uint8x3>(7u);
   verify_simd_stream<cat::uint8x4>(0xffffffff'ffffffffull);
}

$test(splitmix_scalar_batches) {
   verify_scalar_batch<2u>(0u);
   verify_scalar_batch<3u>(1u);
   verify_scalar_batch<4u>(0xffffffff'ffffffffull);
   verify_scalar_batch<4u>(0x61c88646'80b583ebull);
}

$test(splitmix_fill_random_tails) {
   verify_fill_random<0u>(11u);
   verify_fill_random<1u>(11u);
   verify_fill_random<2u>(11u);
   verify_fill_random<3u>(11u);
   verify_fill_random<4u>(11u);
   verify_fill_random<5u>(11u);
   verify_fill_random<7u>(11u);
   verify_fill_random<8u>(11u);
   verify_fill_random<9u>(11u);
   verify_fill_random<15u>(11u);
   verify_fill_random<16u>(11u);
   verify_fill_random<17u>(11u);
}

$test(splitmix_bounded_draws) {
   cat::splitmix64_engine bounded(42u);
   cat::splitmix64_engine reference = bounded;
   for (cat::uint8 bound = 0u; bound < 16u; ++bound) {
      cat::uint8 const expected =
         bound == 0u
            ? reference()
            : cat::detail::lemire_bounded(bound, [&] { return reference(); });
      cat::verify(bounded(bound) == expected);
   }

   for (cat::uint8 minimum = 0u; minimum < 8u; ++minimum) {
      for (cat::uint8 maximum = minimum; maximum < 8u; ++maximum) {
         cat::uint8 const expected =
            minimum
            + cat::detail::lemire_bounded(maximum - minimum + 1u, [&] {
                 return reference();
              });
         cat::verify(bounded(minimum, maximum) == expected);
      }
   }

   cat::splitmix64_engine full_range(42u);
   cat::splitmix64_engine full_range_reference = full_range;
   cat::verify(
      full_range(cat::uint8::min(), cat::uint8::max())
      == full_range_reference()
   );

   cat::splitmix_engine<cat::uint8x4> wide(42u);
   cat::array<cat::splitmix64_engine, 4u> scalar;
   cat::uint8x4 bounds;
   cat::uint8x4 minimums;
   cat::uint8x4 maximums;
   for (cat::idx lane = 0u; lane < 4u; ++lane) {
      scalar[lane].seed(cat::uint8(42u) + lane);
      cat::uint8 bound = 0u;
      if (lane == 1u) {
         bound = 1u;
      } else if (lane > 1u) {
         bound = 0x80000001u + lane.raw;
      }
      bounds.set_lane(lane, bound);
      minimums.set_lane(lane, cat::uint8(lane + 3u));
      maximums.set_lane(lane, cat::uint8(lane + 3u + lane));
   }

   for (cat::idx round = 0u; round < 3u; ++round) {
      cat::uint8x4 const values = wide(bounds);
      for (cat::idx lane = 0u; lane < 4u; ++lane) {
         cat::verify(values[lane] == scalar[lane](bounds[lane]));
      }
   }

   cat::uint8x4 const ranged = wide(minimums, maximums);
   for (cat::idx lane = 0u; lane < 4u; ++lane) {
      cat::verify(
         ranged[lane] == scalar[lane](minimums[lane], maximums[lane])
      );
   }
}
