#include <cat/random>
#include <cat/utility>

#include "../unit_tests.hpp"
#include "pcg_high_expected.hpp"

// `cat::pcg_engine` is tested under an implementation of the full test suite in
// PCG's reference C++ implementation:
// https://github.com/imneme/pcg-cpp/tree/master/test-high

namespace {

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
