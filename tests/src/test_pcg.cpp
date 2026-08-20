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

constexpr unsigned __int128 numpy_seed_zero_state =
   make_uint16(0x1aa1b534'5996452dull, 0x09585eb7'a69561e3ull);
constexpr unsigned __int128 numpy_seed_zero_increment =
   make_uint16(0x418ddadb'3af71a82ull, 0x588133bc'447873a9ull);
constexpr unsigned __int128 numpy_seed_zero_pcg64_next =
   make_uint16(0x3c535930'f580265full, 0xc9b4b5d1'b4aff7d8ull);
constexpr unsigned __int128 numpy_seed_zero_dxsm_next =
   make_uint16(0xf38a4286'a8d82819ull, 0x2d6ec382'12b3b128ull);

constexpr cat::array<cat::uint8, 10u> numpy_seed_zero_pcg64 = {
   0xa30febcf'd9c2825full, 0x4510bdf8'82d9d721ull, 0x0a7d3da9'4ecde8b8ull,
   0x043b27b6'1342f01dull, 0xd0327a78'2cde513bull, 0xe9aa5979'a6401c4eull,
   0x9b4c7b71'80edb27full, 0xbac0495f'f8829a45ull, 0x8b2b01e7'a1dc7fbfull,
   0xef60e807'8f56bfedull,
};

constexpr cat::array<cat::uint8, 10u> numpy_seed_zero_dxsm = {
   0xd97e4a14'7f788a70ull, 0x8dfa7bce'56e3a253ull, 0x13556ed9'f53d3c10ull,
   0x55dbf1c2'41341e98ull, 0xa2cd98f7'22eb0e0aull, 0x083dfc40'7203ade8ull,
   0xeaa083df'518f030dull, 0x44968c87'e432852bull, 0x573107b9'cb8d9eccull,
   0x9eedd1da'50b9dacaull,
};

constexpr unsigned __int128 numpy_testset_state =
   make_uint16(0xdcbf51ec'a8a06d6dull, 0xa11c0e6f'95be4591ull);
constexpr unsigned __int128 numpy_testset_increment =
   make_uint16(0x6f01e045'569d93c4ull, 0x40ac10dd'049310b5ull);

constexpr cat::array<cat::uint8, 12u> numpy_testset_pcg64 = {
   0x60d24054'e17a0698ull, 0xd5e79d89'856e4f12ull, 0xd254972f'e64bd782ull,
   0xf1e3072a'53c72571ull, 0xd7c1d739'3d4115c9ull, 0x77b75928'b763e1e2ull,
   0xee6dee05'190f7909ull, 0x15f7b1c5'1d7fa319ull, 0x27e44105'f26ac2d7ull,
   0x0cc0d88b'29e5b415ull, 0xe07b1a90'c685e361ull, 0xd2e43024'0de95e38ull,
};

constexpr cat::array<cat::uint8, 12u> numpy_testset_dxsm = {
   0xdf1ddcf1'e22521feull, 0xc71b2f9c'706cf151ull, 0x6922a8cc'24ad96b2ull,
   0x82738c54'9beccc30ull, 0x5e8415cd'b1f17580ull, 0x064c54ad'0c09cb43ull,
   0x361a17a6'07dce278ull, 0x4346f6af'b7acad68ull, 0x6e9f14d4'f6398d6bull,
   0xf818d434'3f8ed822ull, 0x6327647d'af508ed6ull, 0xe1d1dbe5'496a262aull,
};

template <typename Engine, cat::idx size>
void
verify_numpy_reference(
   cat::array<cat::uint8, size> const& expected,
   unsigned __int128 initial_state, unsigned __int128 initial_increment
) {
   Engine engine;
   engine.set_state(initial_state);
   engine.set_stream(initial_increment >> 1u);
   cat::verify(engine.state() == initial_state);
   cat::verify(engine.increment() == initial_increment);
   for (cat::uint8 value : expected) {
      cat::verify(engine() == value);
   }
}

template <typename SimdEngine, cat::idx size>
void
verify_numpy_simd_reference(cat::array<cat::uint8, size> const& expected) {
   using result_type = SimdEngine::result_type;
   using state_type = SimdEngine::state_type;
   constexpr cat::idx lanes = result_type::abi_type::lanes;

   SimdEngine engine;
   engine.set_state(state_type(numpy_seed_zero_state));
   engine.set_stream(state_type(numpy_seed_zero_increment >> 1u));

   // SIMD lanes are independent scalar-compatible streams. They are not a
   // flattened NumPy stream.
   for (cat::uint8 expected_value : expected) {
      result_type const values = engine();
      for (cat::idx lane = 0u; lane < lanes; ++lane) {
         cat::verify(values[lane] == expected_value);
      }
   }
}

template <typename Engine>
void
verify_pcg_navigation(Engine original) {
   using state_type = Engine::state_type;
   state_type const short_delta = 257u;
   state_type const long_delta = make_uint16(0x40u, 0x1d3u);

   Engine iterated = original;
   Engine advanced = original;
   for (cat::idx index = 0u; index < 257u; ++index) {
      static_cast<void>(iterated());
   }
   advanced.discard(short_delta);
   cat::verify(advanced == iterated);
   cat::verify((advanced - original) == short_delta);
   advanced.backstep(short_delta);
   cat::verify(advanced == original);

   advanced.discard(long_delta);
   cat::verify((advanced - original) == long_delta);
   advanced.backstep(long_delta);
   cat::verify(advanced == original);

   Engine reversed = original;
   reversed.backstep(long_delta);
   cat::verify((original - reversed) == long_delta);
   reversed.discard(long_delta);
   cat::verify(reversed == original);
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
   engine.backstep(6u);
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

   result_type const discards = cat::simd_iota<result_type>(5u);
   engine.discard(typename SimdEngine::state_type(discards));
   for (cat::idx lane = 0u; lane < lanes; ++lane) {
      scalar[lane].discard(
         static_cast<ScalarEngine::state_type>(discards[lane])
      );
   }
   result_type const discarded = engine();
   for (cat::idx lane = 0u; lane < lanes; ++lane) {
      cat::verify(discarded[lane] == scalar[lane]());
   }

   SimdEngine lane_original(123u, 456u);
   SimdEngine lane_advanced(123u, 456u);
   typename SimdEngine::state_type const lane_deltas(
      cat::simd_iota<result_type>(17u)
   );
   lane_advanced.discard(lane_deltas);
   cat::verify((lane_advanced - lane_original) == lane_deltas);
   lane_advanced.backstep(lane_deltas);
   cat::verify(lane_advanced == lane_original);

   typename SimdEngine::state_type const streams(
      cat::simd_iota<result_type>(700u)
   );
   engine.set_stream(streams);
   cat::verify(engine.stream_id() == streams);
   for (cat::idx lane = 0u; lane < lanes; ++lane) {
      scalar[lane].set_stream(
         static_cast<ScalarEngine::state_type>(cat::uint8(700u) + lane)
      );
   }
   result_type const restreamed = engine();
   for (cat::idx lane = 0u; lane < lanes; ++lane) {
      cat::verify(restreamed[lane] == scalar[lane]());
   }

   SimdEngine original(123u, 456u);
   SimdEngine advanced(123u, 456u);
   cat::verify(original == advanced);
   advanced.discard(11u);
   cat::verify((advanced - original) == typename SimdEngine::state_type(11u));
   advanced.backstep(11u);
   cat::verify(original == advanced);
   static_cast<void>(advanced());
   cat::verify(original != advanced);

   SimdEngine reset(42u, 54u);
   reset.seed();
   cat::verify(reset == SimdEngine());
   reset.seed(42u, 54u);
   cat::verify(reset == SimdEngine(42u, 54u));
   cat::verify(SimdEngine::min() == result_type(0u));
   cat::verify(SimdEngine::max() == result_type(lane_type::max()));
   cat::verify(SimdEngine::streams_pow2() > 0u);
}

template <typename SimdEngine, typename ScalarEngine, cat::pcg_stream stream>
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

   typename SimdEngine::state_type const deltas(
      cat::simd_iota<result_type>(9u)
   );
   SimdEngine original(91u);
   SimdEngine advanced(91u);
   advanced.discard(deltas);
   cat::verify((advanced - original) == deltas);
   advanced.backstep(deltas);
   cat::verify(advanced == original);

   cat::verify(
      SimdEngine::period_pow2()
      == ((sizeof(typename result_type::value_type) * 16u) - (stream == cat::pcg_stream::mcg ? 2u : 0u))
   );
}

template <typename SimdEngine, typename ScalarEngine>
void
verify_pcg_simd_wide_navigation() {
   using result_type = SimdEngine::result_type;
   using state_type = SimdEngine::state_type;
   constexpr cat::idx lanes = result_type::abi_type::lanes;
   result_type const low = cat::simd_iota<result_type>(31u);
   result_type const high = cat::simd_iota<result_type>(1u);
   state_type const deltas(low, high);

   SimdEngine original(42u, 54u);
   SimdEngine advanced(42u, 54u);
   advanced.discard(deltas);
   cat::verify((advanced - original) == deltas);

   cat::array<ScalarEngine, lanes> scalar;
   for (cat::idx lane = 0u; lane < lanes; ++lane) {
      scalar[lane].seed(cat::uint8(42u) + lane, cat::uint8(54u) + lane);
      scalar[lane].discard(make_uint16(high[lane], low[lane]));
   }
   result_type const values = advanced();
   for (cat::idx lane = 0u; lane < lanes; ++lane) {
      cat::verify(values[lane] == scalar[lane]());
   }

   SimdEngine restored(42u, 54u);
   restored.discard(deltas);
   restored.backstep(deltas);
   cat::verify(restored == original);
}

template <typename SimdEngine, typename ScalarEngine>
void
verify_pcg_simd_unique() {
   using result_type = SimdEngine::result_type;
   using state_type = SimdEngine::state_type;
   using lane_type = result_type::value_type;
   constexpr cat::idx lanes = result_type::abi_type::lanes;
   SimdEngine engine(91u);
   state_type const stream_ids = engine.stream_id();
   state_type const increments = engine.increment();
   auto const stream_id_low = stream_ids.low();
   auto const stream_id_high = stream_ids.high();
   auto const increment_low = increments.low();
   cat::array<ScalarEngine, lanes> scalar;
   lane_type previous_stream_id = stream_id_low[0u];

   for (cat::idx lane = 0u; lane < lanes; ++lane) {
      cat::uint8 const sequence = cat::uint8(stream_id_low[lane])
                                  | (cat::uint8(stream_id_high[lane]) << 32u);
      scalar[lane].set_stream(sequence);
      scalar[lane].seed(cat::uint8(91u) + lane);
      cat::verify((increment_low[lane] & 1u) == 1u);
      if (lane != 0u) {
         cat::verify(stream_id_low[lane] == previous_stream_id + 1u);
      }
      previous_stream_id = stream_id_low[lane];
   }

   for (cat::idx draw = 0u; draw < 24u; ++draw) {
      result_type const values = engine();
      for (cat::idx lane = 0u; lane < lanes; ++lane) {
         cat::verify(values[lane] == scalar[lane]());
      }
   }

   SimdEngine const& same = engine;
   cat::verify(same == engine);
   SimdEngine copy = engine;
   cat::verify(copy != engine);
   cat::verify(SimdEngine::streams_pow2() == (sizeof(void*) * 8u) - 1u);
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

$test(pcg_numpy_reference) {
   using pcg64 = cat::pcg_engine<cat::uint8>;
   using pcg64_dxsm = cat::pcg_dxsm_engine<cat::uint8>;

   pcg64 pcg;
   pcg.set_state(numpy_seed_zero_state);
   pcg.set_stream(numpy_seed_zero_increment >> 1u);
   cat::verify(pcg() == numpy_seed_zero_pcg64[0u]);
   cat::verify(pcg.state() == numpy_seed_zero_pcg64_next);

   pcg64_dxsm dxsm;
   dxsm.set_state(numpy_seed_zero_state);
   dxsm.set_stream(numpy_seed_zero_increment >> 1u);
   cat::verify(dxsm() == numpy_seed_zero_dxsm[0u]);
   cat::verify(dxsm.state() == numpy_seed_zero_dxsm_next);

   // These are NumPy's seed-zero post-bootstrap state and raw outputs. State
   // import deliberately does not reproduce NumPy SeedSequence.
   verify_numpy_reference<pcg64>(
      numpy_seed_zero_pcg64, numpy_seed_zero_state, numpy_seed_zero_increment
   );
   verify_numpy_reference<pcg64_dxsm>(
      numpy_seed_zero_dxsm, numpy_seed_zero_state, numpy_seed_zero_increment
   );

   // NumPy's official PCG64 testset-1 and PCG64DXSM testset-1 use seed
   // 0xdeadbeaf. The unusual beaf spelling is intentional and comes directly
   // from NumPy's published test vectors. Raw state import does not reproduce
   // SeedSequence or the historical DXSM bootstrap quirk.
   verify_numpy_reference<pcg64>(
      numpy_testset_pcg64, numpy_testset_state, numpy_testset_increment
   );
   verify_numpy_reference<pcg64_dxsm>(
      numpy_testset_dxsm, numpy_testset_state, numpy_testset_increment
   );
}

$test(pcg_navigation) {
   verify_pcg_navigation(cat::pcg_engine<cat::uint8>(42u, 54u));
   verify_pcg_navigation(
      cat::pcg_engine<cat::uint8, cat::pcg_stream::oneseq>(42u)
   );
   verify_pcg_navigation(
      cat::pcg_engine<cat::uint8, cat::pcg_stream::mcg>(42u)
   );
   verify_pcg_navigation(cat::pcg_dxsm_engine<cat::uint8>(42u, 54u));
   verify_pcg_navigation(
      cat::pcg_dxsm_engine<cat::uint8, cat::pcg_stream::oneseq>(42u)
   );
   verify_pcg_navigation(
      cat::pcg_dxsm_engine<cat::uint8, cat::pcg_stream::mcg>(42u)
   );

   cat::pcg_engine<cat::uint8> original;
   original.set_state(numpy_seed_zero_state);
   original.set_stream(numpy_seed_zero_increment >> 1u);
   auto changed = original;
   changed.set_state(numpy_testset_state);
   cat::verify(changed != original);
   cat::verify(changed.stream_id() == original.stream_id());
   changed.set_state(original.state());
   cat::verify(changed == original);

   unsigned __int128 const state = changed.state();
   changed.set_stream(numpy_testset_increment >> 1u);
   cat::verify(changed.state() == state);
   cat::verify(changed.stream_id() == (numpy_testset_increment >> 1u));
   changed.set_stream(original.stream_id());
   cat::verify(changed == original);
}

$test(pcg_simd_uint4) {
   verify_pcg_simd_setseq<
      cat::pcg_engine<cat::uint4x4>, cat::pcg_engine<cat::uint4>>();
   verify_pcg_simd_setseq<
      cat::pcg_engine<cat::uint4x8>, cat::pcg_engine<cat::uint4>>();
   verify_pcg_simd_single_seed<
      cat::pcg_engine<cat::uint4x8, cat::pcg_stream::oneseq>,
      cat::pcg_engine<cat::uint4, cat::pcg_stream::oneseq>,
      cat::pcg_stream::oneseq>();
   verify_pcg_simd_single_seed<
      cat::pcg_engine<cat::uint4x8, cat::pcg_stream::mcg>,
      cat::pcg_engine<cat::uint4, cat::pcg_stream::mcg>,
      cat::pcg_stream::mcg>();
   verify_pcg_simd_setseq<
      cat::pcg_dxsm_engine<cat::uint4x8>, cat::pcg_dxsm_engine<cat::uint4>>();
   verify_pcg_simd_unique<
      cat::pcg_engine<cat::uint4x4, cat::pcg_stream::unique>,
      cat::pcg_engine<cat::uint4>>();
}

$test(pcg_simd_uint8) {
   verify_pcg_simd_setseq<
      cat::pcg_engine<cat::uint8x2>, cat::pcg_engine<cat::uint8>>();
   verify_pcg_simd_setseq<
      cat::pcg_engine<cat::uint8x4>, cat::pcg_engine<cat::uint8>>();
   verify_pcg_simd_single_seed<
      cat::pcg_engine<cat::uint8x2, cat::pcg_stream::oneseq>,
      cat::pcg_engine<cat::uint8, cat::pcg_stream::oneseq>,
      cat::pcg_stream::oneseq>();
   verify_pcg_simd_single_seed<
      cat::pcg_engine<cat::uint8x4, cat::pcg_stream::oneseq>,
      cat::pcg_engine<cat::uint8, cat::pcg_stream::oneseq>,
      cat::pcg_stream::oneseq>();
   verify_pcg_simd_single_seed<
      cat::pcg_engine<cat::uint8x2, cat::pcg_stream::mcg>,
      cat::pcg_engine<cat::uint8, cat::pcg_stream::mcg>,
      cat::pcg_stream::mcg>();
   verify_pcg_simd_single_seed<
      cat::pcg_engine<cat::uint8x4, cat::pcg_stream::mcg>,
      cat::pcg_engine<cat::uint8, cat::pcg_stream::mcg>,
      cat::pcg_stream::mcg>();
   verify_pcg_simd_setseq<
      cat::pcg_dxsm_engine<cat::uint8x2>, cat::pcg_dxsm_engine<cat::uint8>>();
   verify_pcg_simd_setseq<
      cat::pcg_dxsm_engine<cat::uint8x4>, cat::pcg_dxsm_engine<cat::uint8>>();
   verify_pcg_simd_single_seed<
      cat::pcg_dxsm_engine<cat::uint8x2, cat::pcg_stream::oneseq>,
      cat::pcg_dxsm_engine<cat::uint8, cat::pcg_stream::oneseq>,
      cat::pcg_stream::oneseq>();
   verify_pcg_simd_single_seed<
      cat::pcg_dxsm_engine<cat::uint8x4, cat::pcg_stream::oneseq>,
      cat::pcg_dxsm_engine<cat::uint8, cat::pcg_stream::oneseq>,
      cat::pcg_stream::oneseq>();
   verify_pcg_simd_single_seed<
      cat::pcg_dxsm_engine<cat::uint8x2, cat::pcg_stream::mcg>,
      cat::pcg_dxsm_engine<cat::uint8, cat::pcg_stream::mcg>,
      cat::pcg_stream::mcg>();
   verify_pcg_simd_single_seed<
      cat::pcg_dxsm_engine<cat::uint8x4, cat::pcg_stream::mcg>,
      cat::pcg_dxsm_engine<cat::uint8, cat::pcg_stream::mcg>,
      cat::pcg_stream::mcg>();
   verify_pcg_simd_wide_navigation<
      cat::pcg_engine<cat::uint8x2>, cat::pcg_engine<cat::uint8>>();
   verify_pcg_simd_wide_navigation<
      cat::pcg_engine<cat::uint8x4>, cat::pcg_engine<cat::uint8>>();
   verify_pcg_simd_wide_navigation<
      cat::pcg_dxsm_engine<cat::uint8x2>, cat::pcg_dxsm_engine<cat::uint8>>();
   verify_pcg_simd_wide_navigation<
      cat::pcg_dxsm_engine<cat::uint8x4>, cat::pcg_dxsm_engine<cat::uint8>>();
   verify_numpy_simd_reference<cat::pcg_engine<cat::uint8x2>>(
      numpy_seed_zero_pcg64
   );
   verify_numpy_simd_reference<cat::pcg_engine<cat::uint8x4>>(
      numpy_seed_zero_pcg64
   );
   verify_numpy_simd_reference<cat::pcg_dxsm_engine<cat::uint8x2>>(
      numpy_seed_zero_dxsm
   );
   verify_numpy_simd_reference<cat::pcg_dxsm_engine<cat::uint8x4>>(
      numpy_seed_zero_dxsm
   );
}

$test(pcg_simd_rxs_m_xs) {
   verify_pcg_simd_setseq<
      cat::pcg_engine<cat::uint8x4, cat::pcg_stream::setseq, cat::uint8x4>,
      cat::pcg_engine<cat::uint8, cat::pcg_stream::setseq, cat::uint8>>();
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

$test(pcg_simd_wrapped) {
   using engine_type = cat::pcg_engine<cat::uint4x4, cat::pcg_stream::mcg>;
   engine_type engine(0u);
   cat::verify(engine.wrapped());
   static_cast<void>(engine());
   cat::verify(!engine.wrapped());
   engine.backstep(1u);
   cat::verify(engine.wrapped());
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

   cat::pcg_engine<cat::uint4> restored(123u, 456u);
   cat::pcg_engine<cat::uint4> original(123u, 456u);
   restored.discard(20u);
   restored.backstep(20u);
   cat::verify(restored() == original());

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
   cat::verify((equal_left - equal_right) == 1u);

   cat::pcg_engine<cat::uint4> bounded(42u, 54u);
   cat::verify(bounded(6u) < 6u);
   cat::verify(cat::pcg_engine<cat::uint4>::period_pow2() == 64u);
   cat::verify(
      cat::pcg_engine<cat::uint4, cat::pcg_stream::mcg>::period_pow2() == 62u
   );

   cat::pcg_engine<cat::uint4> streamed(42u, 54u);
   cat::pcg_engine<cat::uint4> restreamed(42u, 1u);
   restreamed.set_stream(54u);
   cat::verify(streamed.stream_id() == restreamed.stream_id());
}
