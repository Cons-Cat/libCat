#include <cat/random>
#include <cat/utility>

#include "../unit_tests.hpp"
#include "pcg_high_expected.hpp"

// `cat::pcg_engine` is tested under an implementation of the full test suite in
// PCG's reference C++ implementation:
// https://github.com/imneme/pcg-cpp/tree/master/test-high

namespace {

constexpr auto
make_uint16(cat::uint8 high, cat::uint8 low) -> unsigned __int128 {
   return (static_cast<unsigned __int128>(high.raw) << 64u) | low.raw;
}

struct pcg_reference_state {
   cat::uint8 low;
   cat::uint8 high;
};

constexpr pcg_reference_state pcg_default_multiplier = {
   4'865'540'595'714'422'341ull,
   2'549'297'995'355'413'924ull,
};
constexpr pcg_reference_state pcg_cheap_multiplier = {
   0xda942042'e4dd58b5ull,
   0u,
};

constexpr auto
pcg_reference_add(pcg_reference_state left, pcg_reference_state right)
   -> pcg_reference_state {
   cat::uint8 const low = left.low + right.low;
   return {
      .low = low,
      .high = left.high + right.high + cat::uint8(low < left.low),
   };
}

constexpr auto
pcg_reference_multiply(pcg_reference_state left, pcg_reference_state right)
   -> pcg_reference_state {
   unsigned __int128 const low_product =
      static_cast<unsigned __int128>(left.low.raw) * right.low.raw;
   return {
      .low = cat::uint8(low_product),
      .high = cat::uint8(low_product >> 64u) + left.high * right.low
              + left.low * right.high,
   };
}

constexpr auto
pcg_reference_increment(pcg_reference_state sequence) -> pcg_reference_state {
   return {
      .low = (sequence.low << 1u) | 1u,
      .high = (sequence.high << 1u) | (sequence.low >> 63u),
   };
}

constexpr auto
pcg_reference_advance(
   pcg_reference_state state, pcg_reference_state delta,
   pcg_reference_state multiplier, pcg_reference_state increment
) -> pcg_reference_state {
   pcg_reference_state accumulated_multiplier{.low = 1u, .high = 0u};
   pcg_reference_state accumulated_increment{.low = 0u, .high = 0u};
   while (delta.low != 0u || delta.high != 0u) {
      if ((delta.low & 1u) != 0u) {
         accumulated_multiplier =
            pcg_reference_multiply(accumulated_multiplier, multiplier);
         accumulated_increment = pcg_reference_add(
            pcg_reference_multiply(accumulated_increment, multiplier), increment
         );
      }
      increment = pcg_reference_multiply(
         pcg_reference_add(multiplier, {1u, 0u}), increment
      );
      multiplier = pcg_reference_multiply(multiplier, multiplier);
      delta.low = (delta.low >> 1u) | (delta.high << 63u);
      delta.high >>= 1u;
   }
   return pcg_reference_add(
      pcg_reference_multiply(accumulated_multiplier, state),
      accumulated_increment
   );
}

constexpr auto
pcg_reference_seed(
   pcg_reference_state initial_state, pcg_reference_state initial_sequence,
   pcg_reference_state multiplier
) -> pcg_reference_state {
   pcg_reference_state const increment =
      pcg_reference_increment(initial_sequence);
   return pcg_reference_advance(
      pcg_reference_add(increment, initial_state), {1u, 0u}, multiplier,
      increment
   );
}

constexpr auto
pcg_reference_xsl_rr(pcg_reference_state state) -> cat::uint8 {
   cat::uint8::raw_type const word = state.high.raw ^ state.low.raw;
   cat::uint8::raw_type const rotation = state.high.raw >> 58u;
   return cat::uint8((word >> rotation) | (word << ((0u - rotation) & 63u)));
}

constexpr auto
pcg_reference_dxsm(pcg_reference_state state) -> cat::uint8 {
   cat::uint8 high = state.high;
   high ^= high >> 32u;
   high *= pcg_cheap_multiplier.low;
   high ^= high >> 48u;
   return high * (state.low | 1u);
}

template <bool dxsm>
constexpr auto
pcg_reference_generate(
   pcg_reference_state& state, pcg_reference_state multiplier,
   pcg_reference_state increment
) -> cat::uint8 {
   if constexpr (dxsm) {
      cat::uint8 const result = pcg_reference_dxsm(state);
      state = pcg_reference_advance(state, {1u, 0u}, multiplier, increment);
      return result;
   } else {
      state = pcg_reference_advance(state, {1u, 0u}, multiplier, increment);
      return pcg_reference_xsl_rr(state);
   }
}

constexpr auto
pcg_reference_native(pcg_reference_state state) -> unsigned __int128 {
   return make_uint16(state.high, state.low);
}

template <typename Engine>
void
verify_pcg_discard(Engine original) {
   cat::uint8 const short_delta = 257u;
   cat::uint8 const long_delta = 0x40000000'000001d3ull;

   Engine iterated = original;
   Engine advanced = original;
   for (cat::idx index = 0u; index < 257u; ++index) {
      static_cast<void>(iterated());
   }
   advanced.discard(short_delta);
   cat::verify(advanced == iterated);

   advanced = original;
   advanced.discard(long_delta);
   Engine composed = original;
   composed.discard(long_delta / 2u);
   composed.discard(long_delta - (long_delta / 2u));
   cat::verify(advanced == composed);
}

template <typename Engine, bool dxsm>
void
verify_pcg_scalar_large_advance() {
   constexpr pcg_reference_state initial_state{
      0xffffffff'fffffff1ull,
      0xfedcba98'76543210ull,
   };
   constexpr pcg_reference_state sequence{
      0x80000000'00000054ull,
      0x13579bdf'2468ace0ull,
   };
   constexpr auto multiplier =
      dxsm ? pcg_cheap_multiplier : pcg_default_multiplier;
   constexpr auto increment = pcg_reference_increment(sequence);
   constexpr cat::array<cat::uint8, 6u> deltas = {
      0xffffffffull,
      0x1'00000000ull,
      0x1'00000001ull,
      0xffffffff'ffffffffull,
      1u,
      0x80000000'00000000ull,
   };

   Engine engine(
      pcg_reference_native(initial_state), pcg_reference_native(sequence)
   );
   pcg_reference_state reference =
      pcg_reference_seed(initial_state, sequence, multiplier);

   for (cat::uint8 delta : deltas) {
      engine.discard(delta);
      reference =
         pcg_reference_advance(reference, {delta, 0u}, multiplier, increment);

      cat::uint8 const expected =
         pcg_reference_generate<dxsm>(reference, multiplier, increment);
      cat::verify(engine() == expected);
   }
}

template <typename Engine, bool dxsm>
void
verify_pcg_simd_large_advance() {
   using result_type = Engine::result_type;
   constexpr cat::idx lanes = result_type::abi_type::lanes;
   constexpr pcg_reference_state initial_state{
      .low = 0xffffffff'fffffff1ull,
      .high = 0xfedcba98'76543210ull,
   };
   constexpr pcg_reference_state initial_sequence{
      .low = 0x80000000'00000054ull,
      .high = 0x13579bdf'2468ace0ull,
   };
   constexpr auto multiplier =
      dxsm ? pcg_cheap_multiplier : pcg_default_multiplier;
   constexpr cat::array<cat::uint8, 6u> deltas = {
      0xffffffffull,          0x1'00000001ull,
      0xffffffff'ffffffffull, 1u,
      0x7fffffff'ffffffffull, 0x80000000'00000000ull,
   };

   Engine engine(
      pcg_reference_native(initial_state),
      pcg_reference_native(initial_sequence)
   );
   cat::array<pcg_reference_state, lanes> references;
   cat::array<pcg_reference_state, lanes> increments;
   for (cat::idx lane = 0u; lane < lanes; ++lane) {
      pcg_reference_state const offset{cat::uint8(lane), 0u};
      pcg_reference_state const state =
         pcg_reference_add(initial_state, offset);
      pcg_reference_state const sequence =
         pcg_reference_add(initial_sequence, offset);
      increments[lane] = pcg_reference_increment(sequence);
      references[lane] = pcg_reference_seed(state, sequence, multiplier);
   }

   for (cat::idx round = 0u; round < 4u; ++round) {
      result_type packed_delta;
      cat::array<pcg_reference_state, lanes> round_deltas;
      for (cat::idx lane = 0u; lane < lanes; ++lane) {
         auto const delta = deltas[(round * lanes + lane) % deltas.size()];
         round_deltas[lane] = {delta, 0u};
         packed_delta.set_lane(lane, delta);
      }

      engine.discard(packed_delta);
      for (cat::idx lane = 0u; lane < lanes; ++lane) {
         references[lane] = pcg_reference_advance(
            references[lane], round_deltas[lane], multiplier, increments[lane]
         );
      }

      result_type const values = engine();
      for (cat::idx lane = 0u; lane < lanes; ++lane) {
         cat::uint8 const expected = pcg_reference_generate<dxsm>(
            references[lane], multiplier, increments[lane]
         );
         cat::verify(values[lane] == expected);
      }
   }
}

template <typename Engine>
auto
pcg_bounded(Engine& engine, typename Engine::result_type bound)
   -> Engine::result_type {
   return engine(bound);
}

template <typename Engine>
void
pcg_shuffle(cat::array<cat::uint1, 52u>& cards, Engine& engine) {
   for (cat::iword last = 51; last > 0; --last) {
      cat::idx const chosen =
         cat::idx(pcg_bounded(engine, typename Engine::result_type(last + 1)));
      cat::swap(cards[chosen], cards[cat::idx(last)]);
   }
}

template <typename Engine, typename Word>
void
verify_pcg_high_round(Engine& engine, pcg_high::round<Word> const& expected) {
   for (cat::idx index = 0u; index < 6u; ++index) {
      cat::verify(engine() == expected.numbers[index]);
   }

   for (cat::idx index = 0u; index < expected.coins.size(); ++index) {
      char const face =
         pcg_bounded(engine, typename Engine::result_type(2u)) ? 'H' : 'T';
      cat::verify(face == expected.coins[index]);
   }

   for (cat::idx index = 0u; index < 33u; ++index) {
      cat::verify(
         pcg_bounded(engine, typename Engine::result_type(6u)) + 1u
         == expected.rolls[index]
      );
   }

   cat::array<cat::uint1, 52u> cards;
   for (cat::idx index = 0u; index < 52u; ++index) {
      cards[index] = index;
   }
   pcg_shuffle(cards, engine);
   cat::verify(cards == expected.cards);
}

template <typename Engine, typename Word, typename... Seeds>
void
verify_pcg_high(
   cat::array<pcg_high::round<Word>, 5u> const& expected, Seeds... seeds
) {
   Engine engine(seeds...);
   for (auto const& round : expected) {
      verify_pcg_high_round(engine, round);
   }
}

template <typename SimdEngine, typename ScalarEngine>
void
verify_pcg_simd_setseq() {
   using result_type = SimdEngine::result_type;
   using lane_type = result_type::value_type;
   constexpr cat::idx lanes = result_type::abi_type::lanes;
   SimdEngine engine(42u, 54u);
   cat::array<ScalarEngine, lanes> scalar;
   for (cat::idx lane = 0u; lane < lanes; ++lane) {
      scalar[lane].seed(cat::uint8(42u) + lane, cat::uint8(54u) + lane);
   }

   for (cat::idx draw = 0u; draw < 32u; ++draw) {
      result_type const values = engine();
      for (cat::idx lane = 0u; lane < lanes; ++lane) {
         cat::verify(values[lane] == scalar[lane]());
      }
   }

   result_type const bounds = cat::simd_iota<result_type>(0u);
   result_type const bounded = engine(bounds);
   for (cat::idx lane = 0u; lane < lanes; ++lane) {
      cat::verify(bounded[lane] == scalar[lane](bounds[lane]));
   }

   result_type const minimums = cat::simd_iota<result_type>(3u);
   result_type const maximums = minimums + cat::simd_iota<result_type>(0u);
   result_type const ranged = engine(minimums, maximums);
   for (cat::idx lane = 0u; lane < lanes; ++lane) {
      cat::verify(ranged[lane] == scalar[lane](minimums[lane], maximums[lane]));
   }

   result_type const discards = cat::simd_iota<result_type>(5u);
   engine.discard(discards);
   for (cat::idx lane = 0u; lane < lanes; ++lane) {
      scalar[lane].discard(cat::uint8(discards[lane]));
   }
   result_type const discarded = engine();
   for (cat::idx lane = 0u; lane < lanes; ++lane) {
      cat::verify(discarded[lane] == scalar[lane]());
   }

   SimdEngine reset(42u, 54u);
   reset.seed();
   cat::verify(reset == SimdEngine());
   reset.seed(42u, 54u);
   cat::verify(reset == SimdEngine(42u, 54u));
   cat::verify(SimdEngine::min() == result_type(0u));
   cat::verify(SimdEngine::max() == result_type(lane_type::max()));
}

template <typename SimdEngine, typename ScalarEngine>
void
verify_pcg_simd_single_seed() {
   using result_type = SimdEngine::result_type;
   constexpr cat::idx lanes = result_type::abi_type::lanes;
   SimdEngine engine(91u);
   cat::array<ScalarEngine, lanes> scalar;
   for (cat::idx lane = 0u; lane < lanes; ++lane) {
      scalar[lane].seed(cat::uint8(91u) + lane);
   }
   for (cat::idx draw = 0u; draw < 24u; ++draw) {
      result_type const values = engine();
      for (cat::idx lane = 0u; lane < lanes; ++lane) {
         cat::verify(values[lane] == scalar[lane]());
      }
   }

   result_type const deltas = cat::simd_iota<result_type>(9u);
   engine.discard(deltas);
   for (cat::idx lane = 0u; lane < lanes; ++lane) {
      scalar[lane].discard(cat::uint8(deltas[lane]));
   }
   result_type const values = engine();
   for (cat::idx lane = 0u; lane < lanes; ++lane) {
      cat::verify(values[lane] == scalar[lane]());
   }
}

template <typename SimdEngine>
void
verify_pcg_simd_unique() {
   using result_type = SimdEngine::result_type;
   constexpr cat::idx lanes = result_type::abi_type::lanes;
   SimdEngine engine(91u);
   result_type const values = engine();
   auto previous = values[0u];
   for (cat::idx lane = 1u; lane < lanes; ++lane) {
      cat::verify(values[lane] != previous);
      previous = values[lane];
   }

   SimdEngine const& same = engine;
   cat::verify(same == engine);
   SimdEngine copy = engine;
   cat::verify(copy != engine);
}

}  // namespace

$test(pcg_uint4) {
   verify_pcg_high<cat::pcg_engine<cat::uint4>>(pcg_high::pcg32, 42u, 54u);
}

$test(pcg_uint4_oneseq) {
   verify_pcg_high<cat::pcg_engine<cat::uint4, cat::pcg_stream::oneseq>>(
      pcg_high::pcg32_oneseq, 42u
   );
}

$test(pcg_uint4_mcg) {
   verify_pcg_high<cat::pcg_engine<cat::uint4, cat::pcg_stream::mcg>>(
      pcg_high::pcg32_fast, 42u
   );
}

$test(pcg_uint8) {
   verify_pcg_high<cat::pcg_engine<cat::uint8>>(pcg_high::pcg64, 42u, 54u);
}

$test(pcg_uint8_oneseq) {
   verify_pcg_high<cat::pcg_engine<cat::uint8, cat::pcg_stream::oneseq>>(
      pcg_high::pcg64_oneseq, 42u
   );
}

$test(pcg_uint8_mcg) {
   verify_pcg_high<cat::pcg_engine<cat::uint8, cat::pcg_stream::mcg>>(
      pcg_high::pcg64_fast, 42u
   );
}

$test(pcg_dxsm_uint4) {
   cat::pcg_dxsm_engine<cat::uint4> engine(42u, 54u);
   cat::verify(engine() == 0x960bd1bfu);
   cat::verify(engine() == 0xe2073b27u);
   cat::verify(engine() == 0x639c0888u);
   cat::verify(engine() == 0x28ac6b18u);
   cat::verify(engine() == 0xc66eeb84u);
   cat::verify(engine() == 0x1f5cd81eu);

   cat::pcg_dxsm_engine<cat::uint4, cat::pcg_stream::oneseq> oneseq(42u);
   cat::verify(oneseq() == 0x03e09689u);
   cat::verify(oneseq() == 0xaf9aa881u);

   cat::pcg_dxsm_engine<cat::uint4, cat::pcg_stream::mcg> fast(42u);
   cat::verify(fast() == 0u);
   cat::verify(fast() == 0x0818e880u);
}

$test(pcg_dxsm_uint8) {
   cat::pcg_dxsm_engine<cat::uint8> engine(42u, 54u);
   cat::verify(engine() == 0xf0847c95'18bddb90ull);
   cat::verify(engine() == 0x8e7d5f55'14ba8aaaull);
   cat::verify(engine() == 0x86fbd36f'8028f6fdull);
   cat::verify(engine() == 0x8d14b6ed'be9f740aull);
   cat::verify(engine() == 0xa85b2896'c7cad55dull);
   cat::verify(engine() == 0x8ca3894a'1d9227bbull);

   cat::pcg_dxsm_engine<cat::uint8, cat::pcg_stream::oneseq> oneseq(42u);
   cat::verify(oneseq() == 0x161fdf2a'9b15ce6full);
   cat::verify(oneseq() == 0x50b321bd'80027795ull);

   cat::pcg_dxsm_engine<cat::uint8, cat::pcg_stream::mcg> fast(42u);
   cat::verify(fast() == 0u);
   cat::verify(fast() == 0x40fcd4c0'c4dd4360ull);

   cat::pcg_dxsm_engine<cat::uint8> advanced(42u, 54u);
   cat::pcg_dxsm_engine<cat::uint8> discarded(42u, 54u);
   advanced.discard(6u);
   for (cat::idx index = 0u; index < 6u; ++index) {
      static_cast<void>(discarded());
   }
   cat::verify(advanced() == discarded());
   cat::verify(
      cat::pcg_dxsm_engine<cat::uint8>(42u, 54u)()
      != cat::pcg_engine<cat::uint8>(42u, 54u)()
   );
}

$test(pcg_increment_shift_regression) {
   // rkern's 2019 fix moved initseq low bit 63 into increment high bit 0.
   // The old emulation tested bit 47 and could set high bit 47 instead.
   // https://github.com/rkern/pcg64/commit/b9604fe16dadfb93eeefe07f4ba1cda9acf7c389
   constexpr unsigned __int128 initial_state =
      make_uint16(0xfedcba98'76543210ull, 0xffffffff'fffffff1ull);
   constexpr unsigned __int128 initial_sequence =
      make_uint16(0x13579bdf'2468ace0ull, 0x80000000'00000054ull);
   cat::pcg_engine<cat::uint8> pcg(initial_state, initial_sequence);
   cat::verify(pcg() == 0x69097423'7023ba37ull);

   // NumPy #22472 records its compatibility seed path using the default
   // multiplier. libCat follows O'Neill's cm_setseq DXSM contract by using
   // the cheap multiplier for bootstrap and outputting the old state.
   // https://github.com/numpy/numpy/issues/22472
   // https://github.com/imneme/pcg-cpp/commit/871d0494ee9c9a7b7c43f753e3d8ca47c26f8005
   cat::pcg_dxsm_engine<cat::uint8> dxsm(initial_state, initial_sequence);
   cat::verify(dxsm() == 0x772998f1'6234317aull);
}

$test(pcg_large_advance_reference) {
   // NumPy #20048 found that emulated delta shifts put high bit 0 in low bit
   // 0 instead of low bit 63. PR #20049 fixed it and PR #20080 backported it.
   // https://github.com/numpy/numpy/issues/20048
   // https://github.com/numpy/numpy/pull/20049
   // https://github.com/numpy/numpy/pull/20080
   verify_pcg_scalar_large_advance<cat::pcg_engine<cat::uint8>, false>();
   verify_pcg_scalar_large_advance<cat::pcg_dxsm_engine<cat::uint8>, true>();
}

$test(pcg_simd_large_advance_reference) {
   // NumPy #20048 and PRs #20049 and #20080 expose the same high-limb shift
   // mistake guarded here in libCat's x2 and x4 two-limb SIMD arithmetic.
   // https://github.com/numpy/numpy/issues/20048
   // https://github.com/numpy/numpy/pull/20049
   // https://github.com/numpy/numpy/pull/20080
   verify_pcg_simd_large_advance<cat::pcg_engine<cat::uint8x2>, false>();
   verify_pcg_simd_large_advance<cat::pcg_engine<cat::uint8x4>, false>();
   verify_pcg_simd_large_advance<cat::pcg_dxsm_engine<cat::uint8x2>, true>();
   verify_pcg_simd_large_advance<cat::pcg_dxsm_engine<cat::uint8x4>, true>();
}

$test(pcg_discard) {
   verify_pcg_discard(cat::pcg_engine<cat::uint8>(42u, 54u));
   verify_pcg_discard(
      cat::pcg_engine<cat::uint8, cat::pcg_stream::oneseq>(42u)
   );
   verify_pcg_discard(cat::pcg_engine<cat::uint8, cat::pcg_stream::mcg>(42u));
   verify_pcg_discard(cat::pcg_dxsm_engine<cat::uint8>(42u, 54u));
   verify_pcg_discard(
      cat::pcg_dxsm_engine<cat::uint8, cat::pcg_stream::oneseq>(42u)
   );
   verify_pcg_discard(
      cat::pcg_dxsm_engine<cat::uint8, cat::pcg_stream::mcg>(42u)
   );
}

$test(pcg_simd_uint4) {
   using uint4x3 =
      cat::simd<cat::uint4, cat::simd_abi::fixed_size<cat::uint4, 3u>>;
   verify_pcg_simd_setseq<
      cat::pcg_engine<uint4x3>, cat::pcg_engine<cat::uint4>>();
   verify_pcg_simd_setseq<
      cat::pcg_engine<cat::uint4x4>, cat::pcg_engine<cat::uint4>>();
   verify_pcg_simd_setseq<
      cat::pcg_engine<cat::uint4x8>, cat::pcg_engine<cat::uint4>>();
   verify_pcg_simd_single_seed<
      cat::pcg_engine<cat::uint4x8, cat::pcg_stream::oneseq>,
      cat::pcg_engine<cat::uint4, cat::pcg_stream::oneseq>>();
   verify_pcg_simd_single_seed<
      cat::pcg_engine<cat::uint4x8, cat::pcg_stream::mcg>,
      cat::pcg_engine<cat::uint4, cat::pcg_stream::mcg>>();
   verify_pcg_simd_setseq<
      cat::pcg_dxsm_engine<cat::uint4x8>, cat::pcg_dxsm_engine<cat::uint4>>();
   verify_pcg_simd_unique<
      cat::pcg_engine<cat::uint4x4, cat::pcg_stream::unique>>();
}

$test(pcg_simd_uint8) {
   verify_pcg_simd_setseq<
      cat::pcg_engine<cat::uint8x2>, cat::pcg_engine<cat::uint8>>();
   verify_pcg_simd_setseq<
      cat::pcg_engine<cat::uint8x4>, cat::pcg_engine<cat::uint8>>();
   verify_pcg_simd_single_seed<
      cat::pcg_engine<cat::uint8x2, cat::pcg_stream::oneseq>,
      cat::pcg_engine<cat::uint8, cat::pcg_stream::oneseq>>();
   verify_pcg_simd_single_seed<
      cat::pcg_engine<cat::uint8x4, cat::pcg_stream::oneseq>,
      cat::pcg_engine<cat::uint8, cat::pcg_stream::oneseq>>();
   verify_pcg_simd_single_seed<
      cat::pcg_engine<cat::uint8x2, cat::pcg_stream::mcg>,
      cat::pcg_engine<cat::uint8, cat::pcg_stream::mcg>>();
   verify_pcg_simd_single_seed<
      cat::pcg_engine<cat::uint8x4, cat::pcg_stream::mcg>,
      cat::pcg_engine<cat::uint8, cat::pcg_stream::mcg>>();
   verify_pcg_simd_setseq<
      cat::pcg_dxsm_engine<cat::uint8x2>, cat::pcg_dxsm_engine<cat::uint8>>();
   verify_pcg_simd_setseq<
      cat::pcg_dxsm_engine<cat::uint8x4>, cat::pcg_dxsm_engine<cat::uint8>>();
   verify_pcg_simd_single_seed<
      cat::pcg_dxsm_engine<cat::uint8x2, cat::pcg_stream::oneseq>,
      cat::pcg_dxsm_engine<cat::uint8, cat::pcg_stream::oneseq>>();
   verify_pcg_simd_single_seed<
      cat::pcg_dxsm_engine<cat::uint8x4, cat::pcg_stream::oneseq>,
      cat::pcg_dxsm_engine<cat::uint8, cat::pcg_stream::oneseq>>();
   verify_pcg_simd_single_seed<
      cat::pcg_dxsm_engine<cat::uint8x2, cat::pcg_stream::mcg>,
      cat::pcg_dxsm_engine<cat::uint8, cat::pcg_stream::mcg>>();
   verify_pcg_simd_single_seed<
      cat::pcg_dxsm_engine<cat::uint8x4, cat::pcg_stream::mcg>,
      cat::pcg_dxsm_engine<cat::uint8, cat::pcg_stream::mcg>>();
}

$test(pcg_signed_arguments) {
   cat::pcg_engine<cat::int4> scalar_signed4(42u, 54u);
   cat::pcg_engine<cat::uint4> scalar_unsigned4(42u, 54u);
   cat::pcg_engine<cat::int8> scalar_signed8(42u, 54u);
   cat::pcg_engine<cat::uint8> scalar_unsigned8(42u, 54u);
   cat::pcg_engine<cat::int4x4> simd_signed4(42u, 54u);
   cat::pcg_engine<cat::uint4x4> simd_unsigned4(42u, 54u);
   cat::pcg_engine<cat::int8x2> simd_signed8(42u, 54u);
   cat::pcg_engine<cat::uint8x2> simd_unsigned8(42u, 54u);

   for (cat::idx draw = 0u; draw < 16u; ++draw) {
      cat::verify(scalar_signed4() == scalar_unsigned4());
      cat::verify(scalar_signed8() == scalar_unsigned8());
      cat::verify(simd_signed4() == simd_unsigned4());
      cat::verify(simd_signed8() == simd_unsigned8());
   }
}

$test(pcg_edge_cases) {
   cat::pcg_engine<cat::uint4> identity(123u, 456u);
   cat::pcg_engine<cat::uint4> advanced(123u, 456u);
   advanced.discard(0u);
   cat::verify(identity() == advanced());

   cat::pcg_engine<cat::uint4> one_step(123u, 456u);
   cat::pcg_engine<cat::uint4> one_call(123u, 456u);
   one_step.discard(1u);
   static_cast<void>(one_call());
   cat::verify(one_step() == one_call());

   cat::pcg_engine<cat::uint4> discarded(123u, 456u);
   cat::pcg_engine<cat::uint4> jumped(123u, 456u);
   jumped.discard(20u);
   for (cat::idx index = 0u; index < 20u; ++index) {
      static_cast<void>(discarded());
   }
   cat::verify(jumped() == discarded());

   cat::pcg_engine<cat::uint8> wide_advanced(123u, 456u);
   cat::pcg_engine<cat::uint8> wide_discarded(123u, 456u);
   wide_advanced.discard(20u);
   for (cat::idx index = 0u; index < 20u; ++index) {
      static_cast<void>(wide_discarded());
   }
   cat::verify(wide_advanced() == wide_discarded());

   cat::pcg_engine<cat::uint4> zero_seq(0u, 0u);
   cat::pcg_engine<cat::uint4> zero_seq_again(0u, 0u);
   cat::verify(zero_seq() == zero_seq_again());
   cat::verify(zero_seq() != cat::pcg_engine<cat::uint4>(0u, 1u)());

   cat::pcg_engine<cat::uint4, cat::pcg_stream::mcg> even(2u);
   cat::pcg_engine<cat::uint4, cat::pcg_stream::mcg> odd(3u);
   cat::verify(even() == odd());
   cat::pcg_engine<cat::uint8, cat::pcg_stream::mcg> even64(2u);
   cat::pcg_engine<cat::uint8, cat::pcg_stream::mcg> odd64(3u);
   cat::verify(even64() == odd64());

   cat::pcg_engine<cat::uint4> setseq(42u, 54u);
   cat::pcg_engine<cat::uint4, cat::pcg_stream::oneseq> oneseq(42u);
   cat::verify(setseq() != oneseq());

   cat::pcg_engine<cat::uint4, cat::pcg_stream::unique> left(42u);
   cat::pcg_engine<cat::uint4, cat::pcg_stream::unique> right(42u);
   cat::verify(left() != right());

   cat::pcg_engine<cat::uint4> equal_left(123u, 456u);
   cat::pcg_engine<cat::uint4> equal_right(123u, 456u);
   cat::verify(equal_left == equal_right);
   static_cast<void>(equal_left());
   cat::verify(equal_left != equal_right);

   cat::pcg_engine<cat::uint4> bounded(42u, 54u);
   cat::verify(bounded(6u) < 6u);

   cat::pcg_engine<cat::uint4> ranged(42u, 54u);
   cat::pcg_engine<cat::uint4> ranged_reference = ranged;
   for (cat::uint4 minimum = 0u; minimum < 16u; ++minimum) {
      for (cat::uint4 maximum = minimum; maximum < 16u; ++maximum) {
         cat::uint4 const expected =
            minimum + ranged_reference(maximum - minimum + 1u);
         cat::verify(ranged(minimum, maximum) == expected);
      }
   }

   cat::pcg_engine<cat::uint4> equal_bound(42u, 54u);
   cat::verify(equal_bound(17u, 17u) == 17u);

   cat::pcg_engine<cat::uint4> full_range(42u, 54u);
   cat::pcg_engine<cat::uint4> full_range_reference = full_range;
   cat::verify(
      full_range(cat::uint4::min(), cat::uint4::max()) == full_range_reference()
   );
}
