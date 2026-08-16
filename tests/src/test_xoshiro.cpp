#include <cat/random>

#include "../unit_tests.hpp"

// Official streams from rust-random `rand_xoshiro` and prng.di.unimi.it
// for explicit state 1, 2, 3, 4.

namespace {

template <typename Engine, cat::idx count, typename... Seeds>
consteval auto
draw_stream(Seeds... seeds) {
   Engine engine(seeds...);
   cat::array<typename Engine::result_type, count> values;
   for (cat::idx index = 0u; index < count; ++index) {
      values[index] = engine();
   }
   return values;
}

template <typename Engine, typename... Seeds>
consteval auto
draw_after_jump(Seeds... seeds) {
   Engine engine(seeds...);
   engine.jump();
   return engine();
}

template <typename Engine, typename... Seeds>
consteval auto
draw_after_long_jump(Seeds... seeds) {
   Engine engine(seeds...);
   engine.long_jump();
   return engine();
}

template <typename Engine, typename Value, cat::idx count>
void
verify_stream(Engine& engine, cat::array<Value, count> const& expected) {
   for (cat::idx index = 0u; index < count; ++index) {
      cat::verify(engine() == expected[index]);
   }
}

constexpr cat::array<cat::uint8, 10u> xoshiro256_plusplus_state{
   0x00000000'02800001ull, 0x00000000'03800067ull, 0x000cc000'03800067ull,
   0x000cc201'994400b2ull, 0x8012a201'9ac433cdull, 0x8a69978a'cdee33baull,
   0xc2711347'33154abdull, 0xac2ba091'79169e97ull, 0xdbf3190a'8f073fd8ull,
   0x9105f14a'b2229220ull
};
constexpr cat::array<cat::uint8, 10u> xoshiro256_starstar_state{
   0x00000000'00002d00ull, 0x00000000'00000000ull, 0x00000000'5a007080ull,
   0x10e00000'00009d80ull, 0x10e0b61c'e1009d80ull, 0x0870021c'e143ad00ull,
   0xe071c3c2'e143f089ull, 0x75a1690e'f7a20380ull, 0x9309685b'465c23f9ull,
   0x284f3cc2'e13e3c88ull
};
constexpr cat::array<cat::uint4, 10u> xoshiro128_plusplus_state{
   0x00000281u, 0x00180387u, 0xc0183387u, 0xd1ae3b02u, 0x31e2310au,
   0xfd275ab0u, 0xe67f7cecu, 0x50d07f0fu, 0x1d896e9bu, 0x2506d3c4u
};

static_assert(
   draw_stream<cat::xoshiro_pp_engine<cat::uint8>, 10u>(1u, 2u, 3u, 4u)
   == xoshiro256_plusplus_state
);
static_assert(
   draw_stream<cat::xoshiro_engine<cat::uint8>, 10u>(1u, 2u, 3u, 4u)
   == xoshiro256_starstar_state
);
static_assert(
   draw_stream<cat::xoshiro_pp_engine<cat::uint4>, 10u>(1u, 2u, 3u, 4u)
   == xoshiro128_plusplus_state
);
static_assert(
   draw_stream<
      cat::xoshiro_engine<cat::uint8, cat::detail::xoshiro_scrambler::plus>,
      1u>(1u, 2u, 3u, 4u)[0u]
   == 5u
);
static_assert(
   draw_stream<
      cat::xoshiro_engine<cat::uint4, cat::detail::xoshiro_scrambler::plus>,
      1u>(1u, 2u, 3u, 4u)[0u]
   == 5u
);
static_assert(
   draw_stream<cat::xoshiro_pp_engine<cat::uint8>, 1u>(0u, 0u, 0u, 0u)[0u]
   == 0x00000000'00800001ull
);
static_assert(
   draw_stream<cat::xoshiro_pp_engine<cat::uint4>, 1u>(0u, 0u, 0u, 0u)[0u]
   == 0x00000081u
);
static_assert(
   draw_after_jump<cat::xoshiro_pp_engine<cat::uint8>>(1u, 2u, 3u, 4u)
   == 0xec879073'673df437ull
);
static_assert(
   draw_after_long_jump<cat::xoshiro_pp_engine<cat::uint8>>(1u, 2u, 3u, 4u)
   == 0xb5c4ea37'0b330bf5ull
);
static_assert(
   draw_after_jump<cat::xoshiro_pp_engine<cat::uint4>>(1u, 2u, 3u, 4u)
   == 0xba8c0ddcu
);
static_assert(
   draw_after_long_jump<cat::xoshiro_pp_engine<cat::uint4>>(1u, 2u, 3u, 4u)
   == 0x99cc2935u
);
static_assert(cat::splitmix64_engine(0u)() == 0xe220a839'7b1dcdafull);

}  // namespace

$test(xoshiro_reference_streams) {
   cat::xoshiro_pp_engine<cat::uint8> plusplus256(1u, 2u, 3u, 4u);
   verify_stream(plusplus256, xoshiro256_plusplus_state);
   cat::xoshiro_engine<cat::uint8> starstar256(1u, 2u, 3u, 4u);
   verify_stream(starstar256, xoshiro256_starstar_state);
   cat::xoshiro_pp_engine<cat::uint4> plusplus128(1u, 2u, 3u, 4u);
   verify_stream(plusplus128, xoshiro128_plusplus_state);

   cat::splitmix64_engine splitmix(0u);
   cat::verify(splitmix() == 0xe220a839'7b1dcdafull);
}

$test(xoshiro_edge_cases) {
   cat::xoshiro_pp_engine<cat::uint8> zero256(0u, 0u, 0u, 0u);
   cat::verify(zero256() == 0x00000000'00800001ull);
   cat::xoshiro_pp_engine<cat::uint4> zero128(0u, 0u, 0u, 0u);
   cat::verify(zero128() == 0x00000081u);

   cat::xoshiro_pp_engine<cat::uint8> seeded_zero(0u);
   cat::verify(seeded_zero() != 0x00000000'00800001ull);

   cat::xoshiro_pp_engine<cat::uint8> plusplus(1u, 2u, 3u, 4u);
   cat::xoshiro_engine<cat::uint8, cat::detail::xoshiro_scrambler::plus> plus(
      1u, 2u, 3u, 4u
   );
   cat::xoshiro_engine<cat::uint8> starstar(1u, 2u, 3u, 4u);
   cat::uint8 const plusplus_value = plusplus();
   cat::uint8 const plus_value = plus();
   cat::uint8 const starstar_value = starstar();
   cat::verify(plusplus_value != plus_value);
   cat::verify(plusplus_value != starstar_value);
   cat::verify(plus_value != starstar_value);

   cat::xoshiro_pp_engine<cat::uint8> jumped(1u, 2u, 3u, 4u);
   cat::xoshiro_pp_engine<cat::uint8> long_jumped(1u, 2u, 3u, 4u);
   cat::xoshiro_pp_engine<cat::uint8> again(1u, 2u, 3u, 4u);
   jumped.jump();
   again.jump();
   long_jumped.long_jump();
   cat::uint8 const jump_value = jumped();
   cat::verify(jump_value == again());
   cat::verify(jump_value == 0xec879073'673df437ull);
   cat::verify(long_jumped() == 0xb5c4ea37'0b330bf5ull);
   cat::verify(jump_value != 0xb5c4ea37'0b330bf5ull);

   cat::xoshiro_pp_engine<cat::uint4> jumped128(1u, 2u, 3u, 4u);
   cat::xoshiro_pp_engine<cat::uint4> long_jumped128(1u, 2u, 3u, 4u);
   jumped128.jump();
   long_jumped128.long_jump();
   cat::verify(jumped128() == 0xba8c0ddcu);
   cat::verify(long_jumped128() == 0x99cc2935u);

   cat::xoshiro_engine<cat::uint8> discarded(1u, 2u, 3u, 4u);
   cat::xoshiro_engine<cat::uint8> stepped(1u, 2u, 3u, 4u);
   discarded.discard(10u);
   for (cat::idx index = 0u; index < 10u; ++index) {
      static_cast<void>(stepped());
   }
   cat::verify(discarded() == stepped());

   cat::xoshiro_engine<cat::uint8> scalar(1u);
   cat::xoshiro_engine<cat::uint8x4> simd(1u);
   cat::verify(simd()[0u] == scalar());
}

$test(xoshiro_arbitrary_jump) {
   cat::xoshiro_engine<cat::uint8> official(1u, 2u, 3u, 4u);
   cat::xoshiro_engine<cat::uint8> by_log(1u, 2u, 3u, 4u);
   official.jump();
   by_log.jump_log2(128u);
   cat::verify(by_log() == official());

   cat::xoshiro_engine<cat::uint8> official_long(1u, 2u, 3u, 4u);
   cat::xoshiro_engine<cat::uint8> by_long_log(1u, 2u, 3u, 4u);
   official_long.long_jump();
   by_long_log.jump_log2(192u);
   cat::verify(by_long_log() == official_long());

   cat::xoshiro_engine<cat::uint4> official128(1u, 2u, 3u, 4u);
   cat::xoshiro_engine<cat::uint4> by_log128(1u, 2u, 3u, 4u);
   official128.jump();
   by_log128.jump_log2(64u);
   cat::verify(by_log128() == official128());
}
