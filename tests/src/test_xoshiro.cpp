#include <cat/random>

#include "../unit_tests.hpp"

// Official streams from rust-random `rand_xoshiro` and prng.di.unimi.it
// for ascending explicit state words.

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

template <typename WideEngine, typename ScalarEngine>
void
verify_wide_stream(cat::uint8 seed) {
   using wide_result = WideEngine::result_type;
   constexpr cat::idx lanes = wide_result::abi_type::lanes;

   WideEngine wide(seed);
   cat::array<ScalarEngine, lanes> scalar;
   for (cat::idx lane = 0u; lane < lanes; ++lane) {
      scalar[lane].seed(seed + cat::uint8(lane) * 0x9e3779b9'7f4a7c15ull);
   }

   for (cat::idx round = 0u; round < 8u; ++round) {
      wide_result const values = wide();
      for (cat::idx lane = 0u; lane < lanes; ++lane) {
         cat::verify(values[lane] == scalar[lane]());
      }
   }
}

template <typename WideEngine, typename ScalarEngine>
void
verify_wide_jumps(cat::uint8 seed) {
   using wide_result = WideEngine::result_type;
   constexpr cat::idx lanes = wide_result::abi_type::lanes;

   WideEngine wide_jump(seed);
   WideEngine wide_long_jump(seed);
   cat::array<ScalarEngine, lanes> scalar_jump;
   cat::array<ScalarEngine, lanes> scalar_long_jump;
   for (cat::idx lane = 0u; lane < lanes; ++lane) {
      scalar_jump[lane].seed(seed + cat::uint8(lane) * 0x9e3779b9'7f4a7c15ull);
      scalar_long_jump[lane] = scalar_jump[lane];
   }

   wide_jump.jump();
   wide_long_jump.long_jump();
   for (cat::idx lane = 0u; lane < lanes; ++lane) {
      scalar_jump[lane].jump();
      scalar_long_jump[lane].long_jump();
   }

   for (cat::idx round = 0u; round < 8u; ++round) {
      wide_result const jumped = wide_jump();
      wide_result const long_jumped = wide_long_jump();
      for (cat::idx lane = 0u; lane < lanes; ++lane) {
         cat::verify(jumped[lane] == scalar_jump[lane]());
         cat::verify(long_jumped[lane] == scalar_long_jump[lane]());
      }
   }
}

template <typename Engine>
void
verify_same_output(Engine& left, Engine& right) {
   using result = Engine::result_type;
   result const left_value = left();
   result const right_value = right();
   if constexpr (cat::is_simd<result>) {
      for (cat::idx lane = 0u; lane < result::abi_type::lanes; ++lane) {
         cat::verify(left_value[lane] == right_value[lane]);
      }
   } else {
      cat::verify(left_value == right_value);
   }
}

template <typename Engine>
void
verify_discard_parity(cat::uint8 seed) {
   Engine discarded(seed);
   Engine stepped(seed);
   discarded.discard(13u);
   for (cat::idx index = 0u; index < 13u; ++index) {
      static_cast<void>(stepped());
   }
   for (cat::idx round = 0u; round < 4u; ++round) {
      verify_same_output(discarded, stepped);
   }
}

template <typename Engine>
void
verify_seed_parity() {
   Engine defaulted;
   Engine explicitly_seeded(1u);
   for (cat::idx round = 0u; round < 4u; ++round) {
      verify_same_output(defaulted, explicitly_seeded);
   }

   defaulted.seed(29u);
   defaulted.discard(7u);
   defaulted.seed();
   Engine reset;
   for (cat::idx round = 0u; round < 4u; ++round) {
      verify_same_output(defaulted, reset);
   }
}

template <typename Left, typename Right>
void
verify_alias_parity(cat::uint8 seed) {
   Left left(seed);
   Right right(seed);
   for (cat::idx round = 0u; round < 8u; ++round) {
      auto const left_value = left();
      auto const right_value = right();
      if constexpr (cat::is_simd<typename Left::result_type>) {
         for (cat::idx lane = 0u; lane < Left::result_type::abi_type::lanes;
              ++lane) {
            cat::verify(left_value[lane] == right_value[lane]);
         }
      } else {
         cat::verify(left_value == right_value);
      }
   }
}

template <typename Engine, typename Value, typename... Seeds>
void
verify_jump_anchors(Value jumped, Value long_jumped, Seeds... seeds) {
   Engine jump_engine(seeds...);
   Engine long_jump_engine(seeds...);
   jump_engine.jump();
   long_jump_engine.long_jump();
   cat::verify(jump_engine() == jumped);
   cat::verify(long_jump_engine() == long_jumped);
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
constexpr cat::array<cat::uint4, 6u> xoroshiro64_star_state{
   0x9e3779bbu, 0x1380cf31u, 0xf233f6b9u,
   0xfde6b3b9u, 0x0f9c9e6cu, 0x0a055d19u
};
constexpr cat::array<cat::uint4, 6u> xoroshiro64_starstar_state{
   0xe2ac153fu, 0x30817eaau, 0x607a3436u,
   0xb030543bu, 0xc1e30385u, 0x435a2fa5u
};
constexpr cat::array<cat::uint4, 6u> xoshiro128_plus_state{
   0x00000005u, 0x00003007u, 0x01803007u,
   0x01a05c0eu, 0x0260840au, 0x43f87e19u
};
constexpr cat::array<cat::uint4, 6u> xoshiro128_starstar_state{
   0x00002d00u, 0x00000000u, 0x005a7080u,
   0x04389d80u, 0x79199d9bu, 0x61963b24u
};
constexpr cat::array<cat::uint8, 6u> xoshiro256_plus_state{
   0x00000000'00000005ull, 0x0000c000'00000007ull, 0x0000c000'18000007ull,
   0x80016000'18040302ull, 0x80619000'24040305ull, 0xc0617014'120f0583ull
};
constexpr cat::array<cat::uint8, 6u> xoroshiro128_plus_state{
   0x00000000'00000003ull, 0x00000060'01030003ull, 0x20c102c3'02000c03ull,
   0x81018067'0d23ad61ull, 0x26d13a49'41333a42ull, 0x538a501c'02f58b2eull
};
constexpr cat::array<cat::uint8, 6u> xoroshiro128_plusplus_state{
   0x00000000'00060001ull, 0x000260c0'00660007ull, 0x180acc04'718606d3ull,
   0x9e226d35'036fc4c7ull, 0x849bc9ac'6b960be4ull, 0x31c5870f'c130361bull
};
constexpr cat::array<cat::uint8, 6u> xoroshiro128_starstar_state{
   0x00000000'00001680ull, 0x00000016'c3804380ull, 0x86b5b3ad'00004380ull,
   0x800044a4'cd1497b2ull, 0x73fe9d66'c77d08f6ull, 0xd9d20b3a'd5023ef0ull
};
constexpr cat::array<cat::uint8, 6u> xoshiro512_plus_state{
   0x00000000'00000004ull, 0x00000000'00000008ull, 0x00000000'00001011ull,
   0x00000000'01801010ull, 0x00003000'01a0401bull, 0x00003400'02a08807ull
};
constexpr cat::array<cat::uint8, 6u> xoshiro512_plusplus_state{
   0x00000000'00080003ull, 0x00000000'00100002ull, 0x00000000'20220004ull,
   0x00000300'20201009ull, 0x60000340'81b6100eull, 0x68003541'11ae2003ull
};
constexpr cat::array<cat::uint8, 6u> xoshiro512_starstar_state{
   0x00000000'00002d00ull, 0x00000000'00000000ull, 0x00000000'00005a00ull,
   0x00000000'01692480ull, 0x00000021'c0004380ull, 0x04380002'd2d00000ull
};
constexpr cat::array<cat::uint8, 6u> xoroshiro1024_star_state{
   0x3c6ef372'fe94f826ull, 0xdaa66d2c'7ddf7439ull, 0x78dde6e5'fd29f04cull,
   0x1715609f'7c746c5full, 0xb54cda58'fbbee872ull, 0x53845412'7b096485ull
};
constexpr cat::array<cat::uint8, 6u> xoroshiro1024_plusplus_state{
   0x00000000'01800001ull, 0x18000030'01800000ull, 0x18000031'82000300ull,
   0x20003041'82800318ull, 0x280031d2'03030418ull, 0x303041e2'83831d20ull
};
constexpr cat::array<cat::uint8, 6u> xoroshiro1024_starstar_state{
   0x00000000'00002d00ull, 0x00000000'00004380ull, 0x00000000'00005a00ull,
   0x00000000'00007080ull, 0x00000000'00008700ull, 0x00000000'00009d80ull
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
static_assert(
   cat::is_same<cat::xoroshiro_engine<cat::int4x4>::result_type, cat::uint4x4>
);
static_assert(
   cat::is_same<cat::xoroshiro_engine<cat::float8x4>::result_type, cat::uint8x4>
);
static_assert(
   cat::is_same<cat::xoshiro512_engine<cat::int8x4>::result_type, cat::uint8x4>
);
static_assert(
   cat::is_same<
      cat::xoroshiro1024_engine<cat::float8x4>::result_type, cat::uint8x4>
);

}  // namespace

$test(xoshiro_reference_streams) {
   cat::xoroshiro_engine<cat::uint4, cat::detail::xoshiro_scrambler::plus>
      star64(1u, 2u);
   verify_stream(star64, xoroshiro64_star_state);
   cat::xoroshiro_engine<cat::uint4> starstar64(1u, 2u);
   verify_stream(starstar64, xoroshiro64_starstar_state);

   cat::xoshiro_engine<cat::uint4, cat::detail::xoshiro_scrambler::plus>
      plus128(1u, 2u, 3u, 4u);
   verify_stream(plus128, xoshiro128_plus_state);
   cat::xoshiro_engine<cat::uint4> starstar128(1u, 2u, 3u, 4u);
   verify_stream(starstar128, xoshiro128_starstar_state);
   cat::xoshiro_pp_engine<cat::uint4> plusplus128(1u, 2u, 3u, 4u);
   verify_stream(plusplus128, xoshiro128_plusplus_state);

   cat::xoshiro_engine<cat::uint8, cat::detail::xoshiro_scrambler::plus>
      plus256(1u, 2u, 3u, 4u);
   verify_stream(plus256, xoshiro256_plus_state);
   cat::xoshiro_pp_engine<cat::uint8> plusplus256(1u, 2u, 3u, 4u);
   verify_stream(plusplus256, xoshiro256_plusplus_state);
   cat::xoshiro_engine<cat::uint8> starstar256(1u, 2u, 3u, 4u);
   verify_stream(starstar256, xoshiro256_starstar_state);

   cat::xoroshiro_engine<cat::uint8, cat::detail::xoshiro_scrambler::plus>
      plus128xor(1u, 2u);
   verify_stream(plus128xor, xoroshiro128_plus_state);
   cat::xoroshiro_pp_engine<cat::uint8> plusplus128xor(1u, 2u);
   verify_stream(plusplus128xor, xoroshiro128_plusplus_state);
   cat::xoroshiro_engine<cat::uint8> starstar128xor(1u, 2u);
   verify_stream(starstar128xor, xoroshiro128_starstar_state);

   cat::xoshiro512_engine<cat::uint8, cat::detail::xoshiro_scrambler::plus>
      plus512(1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u);
   verify_stream(plus512, xoshiro512_plus_state);
   cat::xoshiro512_pp_engine<cat::uint8> plusplus512(
      1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u
   );
   verify_stream(plusplus512, xoshiro512_plusplus_state);
   cat::xoshiro512_engine<cat::uint8> starstar512(
      1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u
   );
   verify_stream(starstar512, xoshiro512_starstar_state);

   cat::xoroshiro1024_engine<cat::uint8, cat::detail::xoshiro_scrambler::star>
      star1024(
         1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u, 16u
      );
   verify_stream(star1024, xoroshiro1024_star_state);
   cat::xoroshiro1024_pp_engine<cat::uint8> plusplus1024(
      1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u, 16u
   );
   verify_stream(plusplus1024, xoroshiro1024_plusplus_state);
   cat::xoroshiro1024_engine<cat::uint8> starstar1024(
      1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u, 16u
   );
   verify_stream(starstar1024, xoroshiro1024_starstar_state);

   cat::splitmix64_engine splitmix(0u);
   cat::verify(splitmix() == 0xe220a839'7b1dcdafull);
}

$test(xoshiro_reference_jumps) {
   verify_jump_anchors<
      cat::xoroshiro_engine<cat::uint4, cat::detail::xoshiro_scrambler::plus>>(
      0x30f52758u, 0x745aacbdu, 1u, 2u
   );
   verify_jump_anchors<cat::xoroshiro_engine<cat::uint4>>(
      0x9938971eu, 0xb8abf666u, 1u, 2u
   );

   verify_jump_anchors<
      cat::xoshiro_engine<cat::uint4, cat::detail::xoshiro_scrambler::plus>>(
      0xac222b77u, 0x1e736ef4u, 1u, 2u, 3u, 4u
   );
   verify_jump_anchors<cat::xoshiro_pp_engine<cat::uint4>>(
      0xba8c0ddcu, 0x99cc2935u, 1u, 2u, 3u, 4u
   );
   verify_jump_anchors<cat::xoshiro_engine<cat::uint4>>(
      0x472fa5a7u, 0xf74b371cu, 1u, 2u, 3u, 4u
   );

   verify_jump_anchors<
      cat::xoshiro_engine<cat::uint8, cat::detail::xoshiro_scrambler::plus>>(
      0x1000ccc0'1af67421ull, 0x3acfeb58'b4b6fff1ull, 1u, 2u, 3u, 4u
   );
   verify_jump_anchors<cat::xoshiro_pp_engine<cat::uint8>>(
      0xec879073'673df437ull, 0xb5c4ea37'0b330bf5ull, 1u, 2u, 3u, 4u
   );
   verify_jump_anchors<cat::xoshiro_engine<cat::uint8>>(
      0xbbd2f312'298443d8ull, 0x527752a1'd792704dull, 1u, 2u, 3u, 4u
   );

   verify_jump_anchors<
      cat::xoroshiro_engine<cat::uint8, cat::detail::xoshiro_scrambler::plus>>(
      0xea081299'd29ad927ull, 0x6786a13d'aa9b187dull, 1u, 2u
   );
   verify_jump_anchors<cat::xoroshiro_pp_engine<cat::uint8>>(
      0x6115ff4c'07d8c03eull, 0xbb077da5'5888837cull, 1u, 2u
   );
   verify_jump_anchors<cat::xoroshiro_engine<cat::uint8>>(
      0x2232b5a1'a6bd6889ull, 0x100714ad'00ea19d8ull, 1u, 2u
   );

   verify_jump_anchors<
      cat::xoshiro512_engine<cat::uint8, cat::detail::xoshiro_scrambler::plus>>(
      0xe28f05fc'9c65b71eull, 0xf2753d74'c8a7305dull, 1u, 2u, 3u, 4u, 5u, 6u,
      7u, 8u
   );
   verify_jump_anchors<cat::xoshiro512_pp_engine<cat::uint8>>(
      0xb86339b7'fc03fec0ull, 0xc5f80dd6'99c67e82ull, 1u, 2u, 3u, 4u, 5u, 6u,
      7u, 8u
   );
   verify_jump_anchors<cat::xoshiro512_engine<cat::uint8>>(
      0x88c63daa'2223c441ull, 0xbcb79f50'c440d4a0ull, 1u, 2u, 3u, 4u, 5u, 6u,
      7u, 8u
   );

   verify_jump_anchors<cat::xoroshiro1024_engine<
      cat::uint8, cat::detail::xoshiro_scrambler::star>>(
      0x40e0d395'abaa1eeaull, 0xdfbfdc85'48267c12ull, 1u, 2u, 3u, 4u, 5u, 6u,
      7u, 8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u, 16u
   );
   verify_jump_anchors<cat::xoroshiro1024_pp_engine<cat::uint8>>(
      0xbb1cbe47'0fb29842ull, 0x0f128418'd5ea7a35ull, 1u, 2u, 3u, 4u, 5u, 6u,
      7u, 8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u, 16u
   );
   verify_jump_anchors<cat::xoroshiro1024_engine<cat::uint8>>(
      0x06a136c7'e8ea4f53ull, 0xe7ff9575'6ab2b97full, 1u, 2u, 3u, 4u, 5u, 6u,
      7u, 8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u, 16u
   );
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

$test(xoshiro_simd_full_lane_streams) {
   verify_wide_stream<
      cat::xoshiro_engine<cat::uint4x4>, cat::xoshiro_engine<cat::uint4>>(7u);
   verify_wide_stream<
      cat::xoshiro_pp_engine<cat::uint4x4>, cat::xoshiro_pp_engine<cat::uint4>>(
      7u
   );
   verify_wide_stream<
      cat::xoshiro_engine<cat::uint8x4>, cat::xoshiro_engine<cat::uint8>>(7u);
   verify_wide_stream<
      cat::xoshiro_pp_engine<cat::uint8x4>, cat::xoshiro_pp_engine<cat::uint8>>(
      7u
   );
   verify_wide_stream<
      cat::xoroshiro_engine<cat::int4x4>, cat::xoroshiro_engine<cat::uint4>>(
      7u
   );
   verify_wide_stream<
      cat::xoroshiro_pp_engine<cat::int4x4>,
      cat::xoroshiro_pp_engine<cat::uint4>>(7u);
   verify_wide_stream<
      cat::xoroshiro_engine<cat::int8x4>, cat::xoroshiro_engine<cat::uint8>>(
      7u
   );
   verify_wide_stream<
      cat::xoroshiro_pp_engine<cat::int8x4>,
      cat::xoroshiro_pp_engine<cat::uint8>>(7u);
   verify_wide_stream<
      cat::xoshiro512_engine<cat::int8x4>, cat::xoshiro512_engine<cat::uint8>>(
      7u
   );
   verify_wide_stream<
      cat::xoshiro512_pp_engine<cat::int8x4>,
      cat::xoshiro512_pp_engine<cat::uint8>>(7u);
   verify_wide_stream<
      cat::xoroshiro1024_engine<cat::int8x4>,
      cat::xoroshiro1024_engine<cat::uint8>>(7u);
   verify_wide_stream<
      cat::xoroshiro1024_pp_engine<cat::int8x4>,
      cat::xoroshiro1024_pp_engine<cat::uint8>>(7u);

   cat::uint4x4 const minimum = cat::xoshiro_engine<cat::uint4x4>::min();
   cat::uint4x4 const maximum = cat::xoshiro_engine<cat::uint4x4>::max();
   for (cat::idx lane = 0u; lane < cat::uint4x4::abi_type::lanes; ++lane) {
      cat::verify(minimum[lane] == 0u);
      cat::verify(maximum[lane] == cat::uint4::max());
   }
}

$test(xoshiro_simd_x2_streams) {
   verify_wide_stream<
      cat::xoshiro_engine<cat::uint4x2>, cat::xoshiro_engine<cat::uint4>>(7u);
   verify_wide_stream<
      cat::xoshiro_pp_engine<cat::uint4x2>, cat::xoshiro_pp_engine<cat::uint4>>(
      7u
   );
   verify_wide_stream<
      cat::xoshiro_engine<cat::uint4x2, cat::detail::xoshiro_scrambler::plus>,
      cat::xoshiro_engine<cat::uint4, cat::detail::xoshiro_scrambler::plus>>(
      7u
   );
   verify_wide_stream<
      cat::xoshiro_engine<cat::uint8x2>, cat::xoshiro_engine<cat::uint8>>(7u);
   verify_wide_stream<
      cat::xoshiro_pp_engine<cat::uint8x2>, cat::xoshiro_pp_engine<cat::uint8>>(
      7u
   );
   verify_wide_stream<
      cat::xoshiro_engine<cat::uint8x2, cat::detail::xoshiro_scrambler::plus>,
      cat::xoshiro_engine<cat::uint8, cat::detail::xoshiro_scrambler::plus>>(
      7u
   );

   verify_wide_stream<
      cat::xoroshiro_engine<cat::uint4x2>, cat::xoroshiro_engine<cat::uint4>>(
      7u
   );
   verify_wide_stream<
      cat::xoroshiro_pp_engine<cat::uint4x2>,
      cat::xoroshiro_pp_engine<cat::uint4>>(7u);
   verify_wide_stream<
      cat::xoroshiro_engine<cat::uint4x2, cat::detail::xoshiro_scrambler::plus>,
      cat::xoroshiro_engine<cat::uint4, cat::detail::xoshiro_scrambler::plus>>(
      7u
   );
   verify_wide_stream<
      cat::xoroshiro_engine<cat::uint8x2>, cat::xoroshiro_engine<cat::uint8>>(
      7u
   );
   verify_wide_stream<
      cat::xoroshiro_pp_engine<cat::uint8x2>,
      cat::xoroshiro_pp_engine<cat::uint8>>(7u);
   verify_wide_stream<
      cat::xoroshiro_engine<cat::uint8x2, cat::detail::xoshiro_scrambler::plus>,
      cat::xoroshiro_engine<cat::uint8, cat::detail::xoshiro_scrambler::plus>>(
      7u
   );

   verify_wide_stream<
      cat::xoshiro512_engine<cat::uint8x2>, cat::xoshiro512_engine<cat::uint8>>(
      7u
   );
   verify_wide_stream<
      cat::xoshiro512_pp_engine<cat::uint8x2>,
      cat::xoshiro512_pp_engine<cat::uint8>>(7u);
   verify_wide_stream<
      cat::xoshiro512_engine<
         cat::uint8x2, cat::detail::xoshiro_scrambler::plus>,
      cat::xoshiro512_engine<cat::uint8, cat::detail::xoshiro_scrambler::plus>>(
      7u
   );
   verify_wide_stream<
      cat::xoroshiro1024_engine<cat::uint8x2>,
      cat::xoroshiro1024_engine<cat::uint8>>(7u);
   verify_wide_stream<
      cat::xoroshiro1024_pp_engine<cat::uint8x2>,
      cat::xoroshiro1024_pp_engine<cat::uint8>>(7u);
   verify_wide_stream<
      cat::xoroshiro1024_engine<
         cat::uint8x2, cat::detail::xoshiro_scrambler::plus>,
      cat::xoroshiro1024_engine<
         cat::uint8, cat::detail::xoshiro_scrambler::plus>>(7u);
}

$test(xoshiro_simd_x2_jumps) {
   verify_wide_jumps<
      cat::xoshiro_engine<cat::uint4x2>, cat::xoshiro_engine<cat::uint4>>(11u);
   verify_wide_jumps<
      cat::xoshiro_pp_engine<cat::uint4x2>, cat::xoshiro_pp_engine<cat::uint4>>(
      11u
   );
   verify_wide_jumps<
      cat::xoshiro_engine<cat::uint4x2, cat::detail::xoshiro_scrambler::plus>,
      cat::xoshiro_engine<cat::uint4, cat::detail::xoshiro_scrambler::plus>>(
      11u
   );
   verify_wide_jumps<
      cat::xoshiro_engine<cat::uint8x2>, cat::xoshiro_engine<cat::uint8>>(11u);
   verify_wide_jumps<
      cat::xoshiro_pp_engine<cat::uint8x2>, cat::xoshiro_pp_engine<cat::uint8>>(
      11u
   );
   verify_wide_jumps<
      cat::xoshiro_engine<cat::uint8x2, cat::detail::xoshiro_scrambler::plus>,
      cat::xoshiro_engine<cat::uint8, cat::detail::xoshiro_scrambler::plus>>(
      11u
   );

   verify_wide_jumps<
      cat::xoroshiro_engine<cat::uint4x2>, cat::xoroshiro_engine<cat::uint4>>(
      11u
   );
   verify_wide_jumps<
      cat::xoroshiro_pp_engine<cat::uint4x2>,
      cat::xoroshiro_pp_engine<cat::uint4>>(11u);
   verify_wide_jumps<
      cat::xoroshiro_engine<cat::uint4x2, cat::detail::xoshiro_scrambler::plus>,
      cat::xoroshiro_engine<cat::uint4, cat::detail::xoshiro_scrambler::plus>>(
      11u
   );
   verify_wide_jumps<
      cat::xoroshiro_engine<cat::uint8x2>, cat::xoroshiro_engine<cat::uint8>>(
      11u
   );
   verify_wide_jumps<
      cat::xoroshiro_pp_engine<cat::uint8x2>,
      cat::xoroshiro_pp_engine<cat::uint8>>(11u);
   verify_wide_jumps<
      cat::xoroshiro_engine<cat::uint8x2, cat::detail::xoshiro_scrambler::plus>,
      cat::xoroshiro_engine<cat::uint8, cat::detail::xoshiro_scrambler::plus>>(
      11u
   );

   verify_wide_jumps<
      cat::xoshiro512_engine<cat::uint8x2>, cat::xoshiro512_engine<cat::uint8>>(
      11u
   );
   verify_wide_jumps<
      cat::xoshiro512_pp_engine<cat::uint8x2>,
      cat::xoshiro512_pp_engine<cat::uint8>>(11u);
   verify_wide_jumps<
      cat::xoshiro512_engine<
         cat::uint8x2, cat::detail::xoshiro_scrambler::plus>,
      cat::xoshiro512_engine<cat::uint8, cat::detail::xoshiro_scrambler::plus>>(
      11u
   );
   verify_wide_jumps<
      cat::xoroshiro1024_engine<cat::uint8x2>,
      cat::xoroshiro1024_engine<cat::uint8>>(11u);
   verify_wide_jumps<
      cat::xoroshiro1024_pp_engine<cat::uint8x2>,
      cat::xoroshiro1024_pp_engine<cat::uint8>>(11u);
   verify_wide_jumps<
      cat::xoroshiro1024_engine<
         cat::uint8x2, cat::detail::xoshiro_scrambler::plus>,
      cat::xoroshiro1024_engine<
         cat::uint8, cat::detail::xoshiro_scrambler::plus>>(11u);
}

$test(xoshiro_simd_full_lane_jumps) {
   verify_wide_jumps<
      cat::xoshiro_engine<cat::uint4x4>, cat::xoshiro_engine<cat::uint4>>(11u);
   verify_wide_jumps<
      cat::xoshiro_pp_engine<cat::uint4x4>, cat::xoshiro_pp_engine<cat::uint4>>(
      11u
   );
   verify_wide_jumps<
      cat::xoshiro_engine<cat::uint8x4>, cat::xoshiro_engine<cat::uint8>>(11u);
   verify_wide_jumps<
      cat::xoshiro_pp_engine<cat::uint8x4>, cat::xoshiro_pp_engine<cat::uint8>>(
      11u
   );
   verify_wide_jumps<
      cat::xoroshiro_engine<cat::int4x4>, cat::xoroshiro_engine<cat::uint4>>(
      11u
   );
   verify_wide_jumps<
      cat::xoroshiro_pp_engine<cat::int4x4>,
      cat::xoroshiro_pp_engine<cat::uint4>>(11u);
   verify_wide_jumps<
      cat::xoroshiro_engine<cat::int8x4>, cat::xoroshiro_engine<cat::uint8>>(
      11u
   );
   verify_wide_jumps<
      cat::xoroshiro_pp_engine<cat::int8x4>,
      cat::xoroshiro_pp_engine<cat::uint8>>(11u);
   verify_wide_jumps<
      cat::xoshiro512_engine<cat::int8x4>, cat::xoshiro512_engine<cat::uint8>>(
      11u
   );
   verify_wide_jumps<
      cat::xoshiro512_pp_engine<cat::int8x4>,
      cat::xoshiro512_pp_engine<cat::uint8>>(11u);
   verify_wide_jumps<
      cat::xoroshiro1024_engine<cat::int8x4>,
      cat::xoroshiro1024_engine<cat::uint8>>(11u);
   verify_wide_jumps<
      cat::xoroshiro1024_pp_engine<cat::int8x4>,
      cat::xoroshiro1024_pp_engine<cat::uint8>>(11u);
}

$test(xoshiro_discard_parity) {
   verify_discard_parity<cat::xoshiro_engine<cat::uint4>>(13u);
   verify_discard_parity<cat::xoshiro_engine<cat::uint8>>(13u);
   verify_discard_parity<cat::xoroshiro_engine<cat::uint4>>(13u);
   verify_discard_parity<cat::xoroshiro_engine<cat::uint8>>(13u);
   verify_discard_parity<cat::xoshiro512_engine<cat::uint8>>(13u);
   verify_discard_parity<cat::xoroshiro1024_engine<cat::uint8>>(13u);

   verify_discard_parity<cat::xoshiro_engine<cat::uint4x2>>(13u);
   verify_discard_parity<cat::xoshiro_engine<cat::uint8x2>>(13u);
   verify_discard_parity<cat::xoroshiro_engine<cat::uint4x2>>(13u);
   verify_discard_parity<cat::xoroshiro_engine<cat::uint8x2>>(13u);
   verify_discard_parity<cat::xoshiro512_engine<cat::uint8x2>>(13u);
   verify_discard_parity<cat::xoroshiro1024_engine<cat::uint8x2>>(13u);
}

$test(xoshiro_seed_reset_and_default_parity) {
   verify_seed_parity<cat::xoshiro_engine<cat::uint4>>();
   verify_seed_parity<cat::xoshiro_engine<cat::uint8>>();
   verify_seed_parity<cat::xoroshiro_engine<cat::uint4>>();
   verify_seed_parity<cat::xoroshiro_engine<cat::uint8>>();
   verify_seed_parity<cat::xoshiro512_engine<cat::uint8>>();
   verify_seed_parity<cat::xoroshiro1024_engine<cat::uint8>>();

   verify_seed_parity<cat::xoshiro_engine<cat::uint4x2>>();
   verify_seed_parity<cat::xoshiro_engine<cat::uint8x2>>();
   verify_seed_parity<cat::xoroshiro_engine<cat::uint4x2>>();
   verify_seed_parity<cat::xoroshiro_engine<cat::uint8x2>>();
   verify_seed_parity<cat::xoshiro512_engine<cat::uint8x2>>();
   verify_seed_parity<cat::xoroshiro1024_engine<cat::uint8x2>>();
}

$test(xoshiro_alias_mapping) {
   verify_alias_parity<
      cat::xoshiro_engine<cat::uint4>,
      cat::xoshiro_engine<
         cat::uint4, cat::detail::xoshiro_scrambler::starstar>>(19u);
   verify_alias_parity<
      cat::xoshiro_pp_engine<cat::uint4>,
      cat::xoshiro_engine<
         cat::uint4, cat::detail::xoshiro_scrambler::plusplus>>(19u);
   verify_alias_parity<
      cat::xoshiro_engine<cat::uint8>,
      cat::xoshiro_engine<
         cat::uint8, cat::detail::xoshiro_scrambler::starstar>>(19u);
   verify_alias_parity<
      cat::xoshiro_pp_engine<cat::uint8>,
      cat::xoshiro_engine<
         cat::uint8, cat::detail::xoshiro_scrambler::plusplus>>(19u);
   verify_alias_parity<
      cat::xoshiro512_engine<cat::uint8>,
      cat::xoshiro512_engine<
         cat::uint8, cat::detail::xoshiro_scrambler::starstar>>(19u);
   verify_alias_parity<
      cat::xoshiro512_pp_engine<cat::uint8>,
      cat::xoshiro512_engine<
         cat::uint8, cat::detail::xoshiro_scrambler::plusplus>>(19u);

   verify_alias_parity<
      cat::xoroshiro_engine<cat::uint4>,
      cat::xoroshiro_engine<
         cat::uint4, cat::detail::xoshiro_scrambler::starstar>>(19u);
   verify_alias_parity<
      cat::xoroshiro_pp_engine<cat::uint4>,
      cat::xoroshiro_engine<
         cat::uint4, cat::detail::xoshiro_scrambler::starstar>>(19u);
   verify_alias_parity<
      cat::xoroshiro_engine<cat::uint4, cat::detail::xoshiro_scrambler::plus>,
      cat::xoroshiro_engine<cat::uint4, cat::detail::xoshiro_scrambler::star>>(
      19u
   );
   verify_alias_parity<
      cat::xoroshiro_engine<cat::uint8>,
      cat::xoroshiro_engine<
         cat::uint8, cat::detail::xoshiro_scrambler::starstar>>(19u);
   verify_alias_parity<
      cat::xoroshiro_pp_engine<cat::uint8>,
      cat::xoroshiro_engine<
         cat::uint8, cat::detail::xoshiro_scrambler::plusplus>>(19u);
   verify_alias_parity<
      cat::xoroshiro_engine<cat::uint8, cat::detail::xoshiro_scrambler::star>,
      cat::xoroshiro_engine<
         cat::uint8, cat::detail::xoshiro_scrambler::starstar>>(19u);
   verify_alias_parity<
      cat::xoroshiro1024_engine<cat::uint8>,
      cat::xoroshiro1024_engine<
         cat::uint8, cat::detail::xoshiro_scrambler::starstar>>(19u);
   verify_alias_parity<
      cat::xoroshiro1024_pp_engine<cat::uint8>,
      cat::xoroshiro1024_engine<
         cat::uint8, cat::detail::xoshiro_scrambler::plusplus>>(19u);
   verify_alias_parity<
      cat::xoroshiro1024_engine<
         cat::uint8, cat::detail::xoshiro_scrambler::plus>,
      cat::xoroshiro1024_pp_engine<cat::uint8>>(19u);

   verify_alias_parity<
      cat::xoroshiro_engine<cat::uint4x2, cat::detail::xoshiro_scrambler::plus>,
      cat::xoroshiro_engine<
         cat::uint4x2, cat::detail::xoshiro_scrambler::star>>(19u);
   verify_alias_parity<
      cat::xoroshiro_engine<cat::uint8x2, cat::detail::xoshiro_scrambler::star>,
      cat::xoroshiro_engine<
         cat::uint8x2, cat::detail::xoshiro_scrambler::starstar>>(19u);
   verify_alias_parity<
      cat::xoroshiro1024_engine<
         cat::uint8x2, cat::detail::xoshiro_scrambler::plus>,
      cat::xoroshiro1024_pp_engine<cat::uint8x2>>(19u);
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
