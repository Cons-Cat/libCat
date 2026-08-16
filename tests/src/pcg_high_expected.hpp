// Generated from imneme/pcg-cpp test-high/expected
#pragma once

#include <cat/array>
#include <cat/string>

namespace pcg_high {

template <typename Word>
struct round {
   cat::array<Word, 6u> numbers;
   cat::str_view coins;
   cat::array<cat::uint1, 33u> rolls;
   cat::array<cat::uint1, 52u> cards;
};

inline constexpr cat::array<round<cat::uint4>, 5u> pcg32{
   round<cat::uint4>{
      .numbers = cat::array<cat::uint4, 6u>(0xa15c02b7u, 0x7b47f409u, 0xba1d3330u, 0x83d2f293u, 0xbfa4784bu, 0xcbed606eu),
      .coins = "HHTTTHTHHHTHTTTHHHHHTTTHHHTHTHTHTTHTTTHHHHHHTTTTHHTTTTTHTTTTTTTHT",
      .rolls = cat::array<cat::uint1, 33u>(3u, 4u, 1u, 1u, 2u, 2u, 3u, 2u, 4u, 3u, 2u, 4u, 3u, 3u, 5u, 2u, 3u, 1u, 3u, 1u, 5u, 1u, 4u, 1u, 5u, 6u, 4u, 6u, 6u, 2u, 6u, 3u, 3u),
      .cards = cat::array<cat::uint1, 52u>(46u, 51u, 22u, 11u, 10u, 13u, 8u, 38u, 49u, 17u, 40u, 50u, 42u, 3u, 15u, 12u, 2u, 36u, 1u, 41u, 27u, 47u, 7u, 24u, 48u, 6u, 21u, 0u, 14u, 44u, 32u, 23u, 19u, 5u, 33u, 39u, 30u, 35u, 9u, 29u, 43u, 18u, 4u, 20u, 26u, 31u, 34u, 16u, 28u, 45u, 25u, 37u),
   },
   round<cat::uint4>{
      .numbers = cat::array<cat::uint4, 6u>(0x74ab93adu, 0x1c1da000u, 0x494ff896u, 0x34462f2fu, 0xd308a3e5u, 0x0fa83babu),
      .coins = "HHHHHHHHHHTHHHTHTHTHTHTTTTHHTTTHHTHHTHTTHHTTTHHHHHHTHTTHTHTTTTTTT",
      .rolls = cat::array<cat::uint1, 33u>(5u, 1u, 1u, 3u, 3u, 2u, 4u, 5u, 3u, 2u, 2u, 6u, 4u, 3u, 2u, 4u, 2u, 4u, 3u, 2u, 3u, 6u, 3u, 2u, 3u, 4u, 2u, 4u, 1u, 1u, 5u, 4u, 4u),
      .cards = cat::array<cat::uint1, 52u>(26u, 7u, 24u, 38u, 31u, 9u, 10u, 43u, 6u, 37u, 12u, 47u, 17u, 33u, 36u, 5u, 41u, 46u, 34u, 45u, 27u, 11u, 19u, 20u, 14u, 40u, 13u, 1u, 15u, 16u, 18u, 49u, 28u, 30u, 42u, 35u, 2u, 23u, 21u, 50u, 4u, 8u, 48u, 39u, 44u, 32u, 22u, 3u, 25u, 51u, 0u, 29u),
   },
   round<cat::uint4>{
      .numbers = cat::array<cat::uint4, 6u>(0x39af5f9fu, 0x04196b18u, 0xc3c3eb28u, 0xc076c60cu, 0xc693e135u, 0xf8f63932u),
      .coins = "HTTHHTTTTTHTTHHHTHTTHHTTHTHHTHTHTTTTHHTTTHHTHHTTHTTHHHTHHHTHTTTHT",
      .rolls = cat::array<cat::uint1, 33u>(5u, 1u, 5u, 3u, 2u, 2u, 4u, 5u, 3u, 3u, 1u, 3u, 4u, 6u, 3u, 2u, 3u, 4u, 2u, 2u, 3u, 1u, 5u, 2u, 4u, 6u, 6u, 4u, 2u, 4u, 3u, 3u, 6u),
      .cards = cat::array<cat::uint1, 52u>(50u, 40u, 49u, 44u, 14u, 45u, 12u, 34u, 9u, 48u, 47u, 28u, 17u, 42u, 26u, 30u, 8u, 25u, 31u, 11u, 4u, 51u, 33u, 32u, 5u, 29u, 2u, 27u, 15u, 7u, 16u, 23u, 13u, 0u, 24u, 19u, 1u, 10u, 18u, 46u, 3u, 37u, 20u, 35u, 6u, 21u, 22u, 38u, 41u, 39u, 36u, 43u),
   },
   round<cat::uint4>{
      .numbers = cat::array<cat::uint4, 6u>(0x55ce6851u, 0x97a7726du, 0x17e10815u, 0x58007d43u, 0x962fb148u, 0xb9bb55bdu),
      .coins = "HHTHHTTTTHTHHHHHTTHHHTTTHHTHTHTHTHHTTHTHHHHHHTHHTHHTHHTTTTHHTHHTT",
      .rolls = cat::array<cat::uint1, 33u>(6u, 6u, 3u, 2u, 3u, 4u, 2u, 6u, 4u, 2u, 6u, 3u, 2u, 3u, 5u, 5u, 3u, 4u, 4u, 6u, 6u, 2u, 6u, 5u, 4u, 4u, 6u, 1u, 6u, 1u, 3u, 6u, 5u),
      .cards = cat::array<cat::uint1, 52u>(46u, 28u, 18u, 31u, 30u, 39u, 24u, 36u, 47u, 43u, 27u, 49u, 20u, 19u, 14u, 1u, 42u, 26u, 25u, 38u, 5u, 23u, 16u, 22u, 11u, 50u, 35u, 40u, 48u, 3u, 0u, 32u, 9u, 44u, 33u, 6u, 37u, 34u, 7u, 10u, 51u, 12u, 45u, 2u, 41u, 29u, 4u, 8u, 15u, 13u, 17u, 21u),
   },
   round<cat::uint4>{
      .numbers = cat::array<cat::uint4, 6u>(0xfcef7cd6u, 0x1b488b5au, 0xd0daf7eau, 0x1d9a70f7u, 0x241a37cfu, 0x9a3857b7u),
      .coins = "HHHHTHHTTHTTHHHTTTHHTHTHTTTTHTTHTHTTTHHHTHTHTTHTTHTHHTHTHHHTHTHTT",
      .rolls = cat::array<cat::uint1, 33u>(5u, 4u, 1u, 2u, 6u, 1u, 3u, 1u, 5u, 6u, 3u, 6u, 2u, 1u, 4u, 4u, 5u, 2u, 1u, 5u, 6u, 5u, 6u, 4u, 4u, 4u, 5u, 2u, 6u, 4u, 3u, 5u, 6u),
      .cards = cat::array<cat::uint1, 52u>(14u, 35u, 45u, 32u, 3u, 47u, 27u, 13u, 50u, 20u, 23u, 5u, 29u, 18u, 24u, 16u, 41u, 11u, 25u, 40u, 43u, 51u, 37u, 42u, 49u, 36u, 8u, 39u, 44u, 2u, 38u, 9u, 0u, 6u, 10u, 17u, 1u, 31u, 19u, 33u, 4u, 21u, 22u, 48u, 46u, 30u, 26u, 7u, 28u, 12u, 34u, 15u),
   },
};

inline cat::array<round<cat::uint4>, 5u> const pcg32_oneseq{
   round<cat::uint4>{
      .numbers = cat::array<cat::uint4, 6u>(0xc2f57bd6u, 0x6b07c4a9u, 0x72b7b29bu, 0x44215383u, 0xf5af5eadu, 0x68beb632u),
      .coins = "THTHHHTTHHTTHTTHTHHHTHTTTHTTHTTHTTTHHTTTTTHHTTTHTTHTHHTHHHTTHTTTH",
      .rolls = cat::array<cat::uint1, 33u>(4u, 1u, 3u, 3u, 6u, 6u, 5u, 1u, 3u, 4u, 4u, 3u, 2u, 2u, 5u, 4u, 1u, 3u, 3u, 3u, 1u, 4u, 6u, 4u, 6u, 6u, 1u, 6u, 1u, 2u, 3u, 6u, 6u),
      .cards = cat::array<cat::uint1, 52u>(6u, 17u, 8u, 22u, 43u, 33u, 12u, 39u, 47u, 18u, 51u, 16u, 2u, 1u, 44u, 36u, 42u, 49u, 37u, 27u, 0u, 50u, 24u, 9u, 14u, 31u, 5u, 10u, 48u, 28u, 41u, 20u, 13u, 30u, 45u, 25u, 38u, 7u, 11u, 15u, 26u, 46u, 40u, 3u, 21u, 29u, 19u, 4u, 23u, 34u, 35u, 32u),
   },
   round<cat::uint4>{
      .numbers = cat::array<cat::uint4, 6u>(0x0573afccu, 0x2cab16dbu, 0x6af6f55au, 0xe916bec2u, 0x1ca9b4a4u, 0xbb2778ebu),
      .coins = "THHHTHTTTHHHTTTTTTHTTHTHTHHHTHHHTHTHTTHTTTTTHTHHTHHTTHHHHHTTTHTTH",
      .rolls = cat::array<cat::uint1, 33u>(1u, 5u, 3u, 3u, 5u, 1u, 5u, 6u, 5u, 6u, 6u, 3u, 5u, 5u, 6u, 6u, 2u, 6u, 4u, 1u, 5u, 6u, 3u, 6u, 5u, 5u, 1u, 3u, 2u, 4u, 5u, 1u, 1u),
      .cards = cat::array<cat::uint1, 52u>(33u, 2u, 18u, 26u, 0u, 29u, 36u, 50u, 17u, 43u, 25u, 49u, 48u, 21u, 51u, 37u, 38u, 10u, 24u, 6u, 19u, 35u, 8u, 3u, 34u, 28u, 15u, 20u, 39u, 5u, 40u, 9u, 31u, 12u, 16u, 23u, 42u, 30u, 11u, 22u, 27u, 14u, 1u, 45u, 13u, 4u, 44u, 32u, 46u, 7u, 47u, 41u),
   },
   round<cat::uint4>{
      .numbers = cat::array<cat::uint4, 6u>(0x114306f3u, 0xb9bf0d91u, 0x1aed8e5eu, 0x587de8b7u, 0x7477c8bdu, 0xd853ec9du),
      .coins = "HTHHTHHHHTHTHTTHTHTHHTHTTHHHTTTTHHTTTTTTHTHTTTHTHTTTHTHHHHTTTTTTT",
      .rolls = cat::array<cat::uint1, 33u>(1u, 5u, 4u, 2u, 1u, 4u, 6u, 3u, 2u, 1u, 6u, 3u, 6u, 4u, 3u, 1u, 4u, 4u, 2u, 5u, 5u, 3u, 3u, 2u, 6u, 1u, 6u, 3u, 2u, 6u, 5u, 6u, 3u),
      .cards = cat::array<cat::uint1, 52u>(0u, 30u, 2u, 42u, 6u, 8u, 40u, 25u, 49u, 51u, 10u, 3u, 15u, 11u, 28u, 45u, 26u, 38u, 21u, 29u, 14u, 17u, 34u, 44u, 43u, 1u, 50u, 19u, 22u, 39u, 32u, 35u, 33u, 5u, 16u, 9u, 18u, 36u, 13u, 23u, 27u, 46u, 24u, 4u, 37u, 20u, 12u, 31u, 47u, 41u, 48u, 7u),
   },
   round<cat::uint4>{
      .numbers = cat::array<cat::uint4, 6u>(0xb982cd46u, 0x01cc6f94u, 0x0ad658aeu, 0xf6c6c97eu, 0xd1b772ddu, 0x0098599eu),
      .coins = "HTTHTTHHHHTHTHHHTTHTHTHTTTHTHTHHTHTHTTTTHHTTHHHTHTTHHTTTHHHTTHHHH",
      .rolls = cat::array<cat::uint1, 33u>(4u, 4u, 5u, 4u, 2u, 1u, 4u, 2u, 2u, 5u, 2u, 5u, 6u, 6u, 2u, 1u, 6u, 6u, 2u, 6u, 6u, 3u, 6u, 2u, 1u, 4u, 1u, 1u, 1u, 1u, 5u, 1u, 5u),
      .cards = cat::array<cat::uint1, 52u>(23u, 38u, 8u, 43u, 24u, 40u, 1u, 48u, 36u, 12u, 9u, 22u, 47u, 0u, 28u, 49u, 37u, 4u, 29u, 5u, 42u, 7u, 44u, 14u, 10u, 51u, 27u, 34u, 18u, 6u, 19u, 16u, 41u, 11u, 35u, 46u, 45u, 26u, 20u, 3u, 31u, 15u, 13u, 30u, 33u, 21u, 17u, 2u, 25u, 32u, 50u, 39u),
   },
   round<cat::uint4>{
      .numbers = cat::array<cat::uint4, 6u>(0xef3c7322u, 0xa1ff2188u, 0x3f564b42u, 0x91c90425u, 0x17711b95u, 0xf43aa1f7u),
      .coins = "HTTHHHTTHTTTHTHHTHTHTHHTHHTTTHTTHTHHTHTTTTTHTHTTHHHHTHTHTHHTHHTHT",
      .rolls = cat::array<cat::uint1, 33u>(4u, 1u, 6u, 3u, 3u, 2u, 5u, 6u, 3u, 2u, 6u, 5u, 3u, 1u, 5u, 5u, 4u, 6u, 4u, 4u, 2u, 5u, 5u, 4u, 1u, 5u, 2u, 4u, 5u, 5u, 5u, 3u, 5u),
      .cards = cat::array<cat::uint1, 52u>(21u, 30u, 14u, 41u, 34u, 3u, 35u, 9u, 33u, 36u, 51u, 47u, 13u, 43u, 0u, 45u, 1u, 50u, 38u, 46u, 48u, 49u, 37u, 42u, 23u, 16u, 29u, 31u, 2u, 19u, 15u, 39u, 8u, 11u, 24u, 26u, 28u, 5u, 6u, 17u, 20u, 4u, 10u, 25u, 32u, 27u, 12u, 7u, 40u, 22u, 44u, 18u),
   },
};

inline cat::array<round<cat::uint4>, 5u> const pcg32_fast{
   round<cat::uint4>{
      .numbers = cat::array<cat::uint4, 6u>(0x00000000u, 0x5c400cccu, 0x03a8459eu, 0x9bdb59c5u, 0xf1c9dcf5u, 0xaac0af3bu),
      .coins = "HTHHTHHTTHTHHHTTTTTHHTTTTHTHTHTTHTTHHHHTHHTTTHHTTTTHTTTTHHHTHTHHT",
      .rolls = cat::array<cat::uint1, 33u>(1u, 3u, 1u, 4u, 3u, 1u, 4u, 3u, 5u, 1u, 5u, 1u, 6u, 3u, 4u, 6u, 2u, 3u, 3u, 5u, 5u, 2u, 5u, 6u, 5u, 3u, 2u, 4u, 2u, 3u, 1u, 1u, 3u),
      .cards = cat::array<cat::uint1, 52u>(7u, 30u, 27u, 32u, 2u, 45u, 40u, 19u, 10u, 11u, 25u, 47u, 48u, 39u, 8u, 1u, 17u, 34u, 23u, 13u, 28u, 6u, 49u, 21u, 33u, 29u, 22u, 18u, 3u, 4u, 24u, 36u, 38u, 43u, 42u, 9u, 16u, 26u, 0u, 46u, 14u, 5u, 51u, 15u, 35u, 41u, 20u, 31u, 12u, 50u, 37u, 44u),
   },
   round<cat::uint4>{
      .numbers = cat::array<cat::uint4, 6u>(0x9d4c8720u, 0x888c050eu, 0x20a18d88u, 0x9af6f5acu, 0xe9e08d16u, 0x30dc8422u),
      .coins = "HTHHHTTTHHHTTHHTTTTTHHHHHHHHHTTHHHHHHTTHHTTTHHTTTHHHTTTHHTHHTHTHH",
      .rolls = cat::array<cat::uint1, 33u>(1u, 6u, 5u, 2u, 2u, 4u, 3u, 6u, 3u, 6u, 4u, 2u, 4u, 3u, 4u, 6u, 6u, 3u, 4u, 4u, 5u, 6u, 4u, 4u, 5u, 4u, 6u, 2u, 1u, 2u, 6u, 2u, 3u),
      .cards = cat::array<cat::uint1, 52u>(29u, 9u, 16u, 6u, 15u, 26u, 12u, 17u, 48u, 19u, 11u, 50u, 32u, 20u, 36u, 24u, 30u, 28u, 47u, 51u, 37u, 44u, 46u, 1u, 34u, 42u, 38u, 22u, 39u, 31u, 43u, 40u, 21u, 25u, 23u, 5u, 49u, 4u, 14u, 33u, 8u, 0u, 45u, 13u, 18u, 27u, 41u, 35u, 3u, 7u, 2u, 10u),
   },
   round<cat::uint4>{
      .numbers = cat::array<cat::uint4, 6u>(0xd9561348u, 0xc5f085ffu, 0x55b15d21u, 0xb00d4c13u, 0x1ad51817u, 0xb1687c92u),
      .coins = "THTHHHTTTHTHHTHTHTTTTHHTTTTHHHTHTTTTTHHTHTHHHHHTTTHTTTTHTTHTTHTHT",
      .rolls = cat::array<cat::uint1, 33u>(5u, 3u, 4u, 1u, 4u, 1u, 5u, 5u, 3u, 1u, 2u, 5u, 4u, 1u, 1u, 6u, 2u, 2u, 1u, 2u, 2u, 2u, 4u, 6u, 3u, 6u, 2u, 4u, 6u, 5u, 2u, 5u, 1u),
      .cards = cat::array<cat::uint1, 52u>(26u, 16u, 3u, 29u, 44u, 22u, 39u, 46u, 0u, 21u, 25u, 33u, 30u, 32u, 15u, 13u, 10u, 14u, 2u, 51u, 38u, 11u, 9u, 50u, 28u, 49u, 42u, 48u, 36u, 47u, 40u, 31u, 5u, 27u, 41u, 4u, 20u, 34u, 6u, 35u, 1u, 7u, 45u, 19u, 43u, 18u, 37u, 17u, 12u, 8u, 23u, 24u),
   },
   round<cat::uint4>{
      .numbers = cat::array<cat::uint4, 6u>(0xb00d4873u, 0x97e247d1u, 0x3aed3e74u, 0xa6f02f6au, 0x007428aeu, 0x88fb2312u),
      .coins = "HTTHTHTTTHHTHHTTHHTTHTHHTTTHTHTHHHTHTHTTTHHHHTHHTHHHHTTHHHHTHHHHT",
      .rolls = cat::array<cat::uint1, 33u>(2u, 5u, 2u, 5u, 3u, 4u, 5u, 6u, 4u, 3u, 3u, 3u, 2u, 2u, 2u, 3u, 3u, 3u, 2u, 6u, 1u, 2u, 1u, 3u, 5u, 4u, 6u, 6u, 6u, 3u, 4u, 5u, 1u),
      .cards = cat::array<cat::uint1, 52u>(15u, 45u, 7u, 12u, 48u, 22u, 39u, 35u, 5u, 51u, 16u, 41u, 4u, 1u, 47u, 42u, 30u, 6u, 27u, 9u, 29u, 21u, 50u, 49u, 24u, 13u, 32u, 8u, 19u, 0u, 28u, 25u, 38u, 10u, 20u, 18u, 37u, 44u, 11u, 14u, 33u, 2u, 46u, 34u, 31u, 40u, 26u, 17u, 3u, 43u, 36u, 23u),
   },
   round<cat::uint4>{
      .numbers = cat::array<cat::uint4, 6u>(0x9b08b727u, 0x4b6859afu, 0x06de6f08u, 0x628f4193u, 0x39397e2du, 0x9e8304d1u),
      .coins = "THTHHTTTTHTTHHHTHTHTHTHHTTTHHHHTHHHTTTHHTHTHHTHTHTTTTTHHHHHHTTTHT",
      .rolls = cat::array<cat::uint1, 33u>(6u, 1u, 4u, 1u, 3u, 4u, 5u, 6u, 5u, 1u, 2u, 6u, 3u, 3u, 6u, 4u, 6u, 5u, 2u, 1u, 3u, 6u, 3u, 3u, 1u, 5u, 5u, 3u, 6u, 2u, 2u, 2u, 2u),
      .cards = cat::array<cat::uint1, 52u>(13u, 4u, 44u, 22u, 20u, 28u, 19u, 46u, 21u, 26u, 1u, 16u, 31u, 2u, 14u, 35u, 5u, 37u, 43u, 10u, 36u, 0u, 42u, 18u, 8u, 15u, 39u, 49u, 23u, 30u, 29u, 6u, 48u, 24u, 33u, 45u, 9u, 38u, 3u, 25u, 50u, 7u, 34u, 51u, 40u, 41u, 17u, 12u, 11u, 47u, 32u, 27u),
   },
};

inline cat::array<round<cat::uint8>, 5u> const pcg64{
   round<cat::uint8>{
      .numbers = cat::array<cat::uint8, 6u>(0x86b1da1d'72062b68ull, 0x1304aa46'c9853d39ull, 0xa3670e9e'0dd50358ull, 0xf9090e52'9a7dae00ull, 0xc85b9fd8'37996f2cull, 0x606121f8'e3919196ull),
      .coins = "TTTHHHTTTHHHTTTTHHTTHHTHTHTTHHTHTTTTHHTTTHTHHTHTTTTHHTTTHHHTTTHTT",
      .rolls = cat::array<cat::uint1, 33u>(6u, 4u, 1u, 5u, 1u, 5u, 5u, 3u, 6u, 3u, 4u, 6u, 2u, 3u, 6u, 5u, 5u, 5u, 1u, 5u, 3u, 6u, 2u, 6u, 1u, 4u, 4u, 3u, 5u, 2u, 6u, 3u, 2u),
      .cards = cat::array<cat::uint1, 52u>(10u, 26u, 8u, 46u, 34u, 29u, 39u, 2u, 35u, 21u, 40u, 1u, 19u, 13u, 5u, 27u, 48u, 50u, 24u, 44u, 22u, 45u, 30u, 47u, 23u, 43u, 14u, 49u, 32u, 9u, 4u, 38u, 18u, 16u, 33u, 15u, 17u, 25u, 11u, 12u, 3u, 36u, 20u, 41u, 7u, 42u, 37u, 0u, 6u, 51u, 28u, 31u),
   },
   round<cat::uint8>{
      .numbers = cat::array<cat::uint8, 6u>(0x1773ba24'1e7a792aull, 0xe41aed71'17b0bc10ull, 0x36bac8d9'432af525ull, 0xe0c78e2f'3c850a38ull, 0xe3ad939c'1c7ce70dull, 0xa302fdce'd8c79e93ull),
      .coins = "TTTTHTHTHHTHTHTTTTTHHTTHHHHTHTHHHHHHHTHHHTHHTHTTTHHHHTTHHTTTHTHTH",
      .rolls = cat::array<cat::uint1, 33u>(6u, 1u, 1u, 5u, 4u, 1u, 5u, 6u, 3u, 2u, 4u, 2u, 2u, 4u, 6u, 2u, 1u, 5u, 2u, 6u, 2u, 3u, 1u, 5u, 1u, 1u, 5u, 4u, 4u, 2u, 3u, 6u, 3u),
      .cards = cat::array<cat::uint1, 52u>(3u, 4u, 14u, 26u, 2u, 45u, 35u, 24u, 48u, 41u, 25u, 10u, 29u, 36u, 33u, 46u, 32u, 38u, 22u, 30u, 47u, 17u, 23u, 31u, 1u, 50u, 6u, 8u, 44u, 37u, 40u, 0u, 11u, 12u, 34u, 28u, 42u, 15u, 7u, 39u, 19u, 49u, 13u, 18u, 9u, 20u, 5u, 21u, 27u, 43u, 16u, 51u),
   },
   round<cat::uint8>{
      .numbers = cat::array<cat::uint8, 6u>(0xc9600659'3aed3b62ull, 0xf04d5afa'3f197bf1ull, 0xce6f729c'c913a50full, 0x98b5fc4f'bb1e4aeaull, 0x802dce1b'410fc8c3ull, 0xe3bac0a1'4f6e5033ull),
      .coins = "HTTHTHTTTTTHTTTHHTHTHHTHHHHHHHHHTTTHTHTHTHHTTTTTTHHHHTHTTTTHHHHHH",
      .rolls = cat::array<cat::uint1, 33u>(5u, 6u, 4u, 3u, 3u, 1u, 4u, 5u, 2u, 3u, 2u, 1u, 1u, 3u, 2u, 3u, 4u, 5u, 4u, 6u, 4u, 3u, 6u, 2u, 2u, 6u, 3u, 2u, 2u, 4u, 5u, 2u, 5u),
      .cards = cat::array<cat::uint1, 52u>(17u, 18u, 34u, 15u, 47u, 48u, 5u, 8u, 1u, 7u, 27u, 13u, 23u, 28u, 33u, 22u, 4u, 14u, 9u, 16u, 20u, 2u, 25u, 43u, 42u, 21u, 6u, 10u, 12u, 50u, 35u, 36u, 49u, 24u, 31u, 37u, 45u, 46u, 40u, 51u, 30u, 39u, 0u, 41u, 19u, 3u, 44u, 29u, 11u, 38u, 26u, 32u),
   },
   round<cat::uint8>{
      .numbers = cat::array<cat::uint8, 6u>(0x68da679d'e81de48aull, 0x7ee3c031'fa0aa440ull, 0x6eb16639'83530403ull, 0xfec4d7a9'a7aec823ull, 0xbce221c2'55ee9467ull, 0x460a42a9'62b8a2f9ull),
      .coins = "HHHTTTTHHHHHTTTTTTTHHHTHHHHTTHTTTHTTTTHTHHHHTHHTTTHHHTHHTTHHHTHTH",
      .rolls = cat::array<cat::uint1, 33u>(3u, 5u, 6u, 3u, 6u, 4u, 5u, 6u, 5u, 6u, 1u, 1u, 6u, 6u, 5u, 5u, 5u, 1u, 6u, 4u, 6u, 4u, 5u, 1u, 1u, 4u, 4u, 4u, 3u, 5u, 6u, 1u, 6u),
      .cards = cat::array<cat::uint1, 52u>(25u, 48u, 6u, 45u, 40u, 43u, 49u, 51u, 50u, 10u, 30u, 15u, 41u, 29u, 34u, 17u, 33u, 44u, 3u, 46u, 11u, 1u, 8u, 9u, 2u, 32u, 20u, 36u, 42u, 19u, 23u, 24u, 26u, 27u, 5u, 4u, 7u, 22u, 28u, 14u, 39u, 37u, 12u, 16u, 13u, 0u, 35u, 38u, 31u, 18u, 21u, 47u),
   },
   round<cat::uint8>{
      .numbers = cat::array<cat::uint8, 6u>(0x9e0d084c'ff42fe2full, 0x63cd8347'aae338eaull, 0x112aae00'540d3fa1ull, 0x53968bc8'29afd6ecull, 0x1b9900eb'6c5b6d90ull, 0xe89ed17e'a33cb420ull),
      .coins = "HTTTTTHTHTHHHTHTTTHTHHTHHTHTTTHHTTHHHTTTTHTTHHTHHTHHHTTHHTHTHHHHH",
      .rolls = cat::array<cat::uint1, 33u>(6u, 6u, 5u, 1u, 1u, 4u, 5u, 5u, 3u, 1u, 2u, 6u, 5u, 2u, 4u, 6u, 4u, 2u, 6u, 4u, 4u, 3u, 2u, 5u, 3u, 3u, 6u, 5u, 3u, 4u, 5u, 1u, 2u),
      .cards = cat::array<cat::uint1, 52u>(42u, 44u, 31u, 32u, 48u, 9u, 39u, 36u, 49u, 50u, 15u, 0u, 16u, 14u, 41u, 26u, 33u, 1u, 29u, 51u, 23u, 6u, 38u, 45u, 7u, 28u, 37u, 21u, 10u, 8u, 12u, 20u, 27u, 47u, 3u, 18u, 11u, 17u, 22u, 13u, 43u, 19u, 30u, 34u, 5u, 35u, 24u, 46u, 40u, 2u, 4u, 25u),
   },
};

inline cat::array<round<cat::uint8>, 5u> const pcg64_oneseq{
   round<cat::uint8>{
      .numbers = cat::array<cat::uint8, 6u>(0x287472e8'7ff5705aull, 0xbbd190b0'4ed0b545ull, 0xb6cee358'0db14880ull, 0xbf5f7d7e'4c3d1864ull, 0x734eedbe'7e50bbc5ull, 0xa5b6b5f8'67691c77ull),
      .coins = "HHTHHHTTHTHHTTTHHHTHHHTTTTTHHTHTTTTHHTHHTTTTTTHHTHTHTTTTTHTHHTTHT",
      .rolls = cat::array<cat::uint1, 33u>(1u, 2u, 6u, 3u, 6u, 2u, 6u, 5u, 3u, 2u, 3u, 2u, 1u, 5u, 1u, 6u, 1u, 3u, 3u, 5u, 4u, 3u, 1u, 5u, 1u, 4u, 6u, 4u, 1u, 6u, 5u, 5u, 5u),
      .cards = cat::array<cat::uint1, 52u>(33u, 16u, 26u, 25u, 13u, 30u, 24u, 45u, 48u, 6u, 8u, 4u, 46u, 39u, 10u, 49u, 32u, 41u, 20u, 22u, 29u, 14u, 44u, 3u, 40u, 31u, 36u, 19u, 5u, 34u, 1u, 12u, 50u, 18u, 35u, 21u, 11u, 51u, 43u, 42u, 27u, 7u, 9u, 37u, 47u, 15u, 28u, 17u, 0u, 23u, 38u, 2u),
   },
   round<cat::uint8>{
      .numbers = cat::array<cat::uint8, 6u>(0x7d97ee72'fb94fdf0ull, 0xb35f07d5'3cc42b66ull, 0x0854c5ca'ec0c251full, 0xf37961a6'45554320ull, 0x1d1d2136'22351b24ull, 0x6edbb396'c73fb49full),
      .coins = "HHTTHHHTHHHHHTTHHHTHHTHTHTTTHHTHHHHHHTHTTHHHHHTHTTHTHHHTHHTTHHHHH",
      .rolls = cat::array<cat::uint1, 33u>(5u, 4u, 2u, 2u, 5u, 3u, 2u, 2u, 2u, 4u, 3u, 1u, 2u, 5u, 6u, 1u, 6u, 5u, 3u, 1u, 1u, 3u, 5u, 6u, 4u, 2u, 5u, 3u, 1u, 2u, 2u, 4u, 1u),
      .cards = cat::array<cat::uint1, 52u>(44u, 0u, 9u, 22u, 31u, 8u, 40u, 29u, 23u, 32u, 30u, 11u, 33u, 39u, 46u, 49u, 48u, 14u, 25u, 16u, 36u, 7u, 5u, 13u, 20u, 42u, 38u, 6u, 51u, 17u, 24u, 47u, 10u, 15u, 26u, 19u, 34u, 1u, 21u, 35u, 28u, 3u, 41u, 27u, 4u, 37u, 50u, 12u, 2u, 18u, 45u, 43u),
   },
   round<cat::uint8>{
      .numbers = cat::array<cat::uint8, 6u>(0x187ee004'30cec695ull, 0x38efe3fb'60c70613ull, 0x3949bd01'ef38c552ull, 0xd3f1543a'45f3b48full, 0xfb81a048'2dc602cdull, 0xb48e4f66'1e4c7fc5ull),
      .coins = "THTTTHHTHHHHHHTHTTTTHHTHTHTHHHTHTTTTTHHTHTTTTTHTTTTTHHTHHHTTTTHHT",
      .rolls = cat::array<cat::uint1, 33u>(1u, 5u, 6u, 2u, 1u, 3u, 2u, 5u, 5u, 6u, 3u, 4u, 4u, 1u, 5u, 3u, 5u, 1u, 4u, 1u, 2u, 4u, 4u, 1u, 3u, 5u, 6u, 5u, 4u, 5u, 5u, 6u, 3u),
      .cards = cat::array<cat::uint1, 52u>(33u, 15u, 34u, 48u, 43u, 21u, 14u, 28u, 46u, 30u, 19u, 10u, 20u, 27u, 35u, 51u, 7u, 24u, 1u, 2u, 49u, 13u, 16u, 42u, 3u, 0u, 31u, 4u, 40u, 23u, 36u, 5u, 45u, 29u, 18u, 32u, 8u, 12u, 44u, 11u, 37u, 17u, 6u, 47u, 9u, 39u, 50u, 41u, 25u, 22u, 26u, 38u),
   },
   round<cat::uint8>{
      .numbers = cat::array<cat::uint8, 6u>(0xd04c0a3a'8cf6c571ull, 0xbc94812f'e9ec2c93ull, 0x691f3e3a'a2f42c77ull, 0xb7188d51'62d89a1eull, 0x17fbf02e'08fee28aull, 0x1aa17486'e288664full),
      .coins = "THTHHHTTTHHHHHTTHHHHHTTHHTTHTHTTHHHHHTHTTHTTTTHTTTHTHHHHTTHHTTHTH",
      .rolls = cat::array<cat::uint1, 33u>(3u, 6u, 5u, 6u, 2u, 3u, 5u, 6u, 2u, 5u, 1u, 5u, 3u, 5u, 6u, 2u, 3u, 1u, 1u, 3u, 1u, 6u, 6u, 4u, 4u, 5u, 4u, 4u, 4u, 2u, 4u, 6u, 5u),
      .cards = cat::array<cat::uint1, 52u>(22u, 13u, 33u, 27u, 47u, 44u, 18u, 16u, 42u, 30u, 9u, 10u, 32u, 36u, 26u, 37u, 3u, 24u, 35u, 6u, 7u, 38u, 31u, 15u, 25u, 46u, 41u, 17u, 19u, 5u, 4u, 23u, 21u, 12u, 43u, 51u, 28u, 20u, 40u, 48u, 14u, 11u, 0u, 34u, 8u, 49u, 29u, 45u, 50u, 39u, 2u, 1u),
   },
   round<cat::uint8>{
      .numbers = cat::array<cat::uint8, 6u>(0x10bda17a'1292d5aaull, 0xf0cd1384'e25b3497ull, 0x8e592be4'9a6a6181ull, 0x5edc4faf'5cda5865ull, 0xb2ecea43'437a3f8cull, 0x98dbb99c'3550f0e4ull),
      .coins = "HTHTHHHHHHTHHTTHHHTTTTTTHTTTHHHTHHHHHHHHHTTTTHHTTTHTHHHHTTHHHHTTH",
      .rolls = cat::array<cat::uint1, 33u>(5u, 1u, 4u, 6u, 1u, 4u, 2u, 4u, 2u, 1u, 3u, 2u, 4u, 3u, 6u, 3u, 5u, 5u, 4u, 5u, 1u, 2u, 1u, 1u, 1u, 6u, 5u, 6u, 5u, 4u, 1u, 6u, 4u),
      .cards = cat::array<cat::uint1, 52u>(10u, 24u, 42u, 8u, 18u, 7u, 26u, 19u, 34u, 31u, 13u, 29u, 41u, 0u, 37u, 43u, 2u, 16u, 20u, 36u, 27u, 50u, 46u, 32u, 3u, 30u, 9u, 6u, 5u, 48u, 35u, 25u, 1u, 21u, 33u, 4u, 47u, 12u, 39u, 23u, 51u, 40u, 17u, 28u, 11u, 22u, 49u, 14u, 38u, 44u, 45u, 15u),
   },
};

inline cat::array<round<cat::uint8>, 5u> const pcg64_fast{
   round<cat::uint8>{
      .numbers = cat::array<cat::uint8, 6u>(0x63b4a3a8'13ce700aull, 0x38295420'0617ab24ull, 0xa7fd85ae'3fe950ceull, 0xd715286a'a2887737ull, 0x60c92fee'2e59f32cull, 0x84c4e96b'eff30017ull),
      .coins = "HTTTTTTTTTTTHHTHHHHHHTTHTHTTHTTTTTHTTHTTHHHHTHHTHHHHHTTTHHHHHHHHH",
      .rolls = cat::array<cat::uint1, 33u>(4u, 5u, 5u, 4u, 4u, 4u, 5u, 4u, 3u, 1u, 6u, 1u, 6u, 6u, 2u, 6u, 2u, 2u, 3u, 3u, 2u, 5u, 6u, 4u, 2u, 6u, 4u, 4u, 3u, 2u, 4u, 2u, 6u),
      .cards = cat::array<cat::uint1, 52u>(40u, 41u, 4u, 12u, 50u, 5u, 14u, 17u, 49u, 29u, 24u, 38u, 35u, 15u, 7u, 42u, 31u, 20u, 46u, 22u, 33u, 44u, 37u, 8u, 25u, 6u, 13u, 11u, 45u, 19u, 30u, 34u, 0u, 1u, 9u, 23u, 16u, 27u, 51u, 36u, 18u, 2u, 43u, 48u, 32u, 47u, 21u, 10u, 26u, 28u, 3u, 39u),
   },
   round<cat::uint8>{
      .numbers = cat::array<cat::uint8, 6u>(0x824eb71d'0f02dfb4ull, 0x7aaad637'30e335c1ull, 0xf87271e1'97a74023ull, 0x86d11317'e615a346ull, 0xe0671479'98450163ull, 0x59fc1336'8ae72993ull),
      .coins = "TTHHTHTHTTTHTTTHHHTTTHTTHTHHHTTHHTHHTHHTHTTHTTHTHHHTTTTHTHTTTTTHH",
      .rolls = cat::array<cat::uint1, 33u>(1u, 5u, 2u, 5u, 1u, 5u, 5u, 3u, 3u, 3u, 5u, 6u, 3u, 5u, 2u, 4u, 1u, 2u, 1u, 4u, 6u, 2u, 5u, 2u, 2u, 4u, 3u, 6u, 5u, 1u, 2u, 5u, 5u),
      .cards = cat::array<cat::uint1, 52u>(36u, 47u, 5u, 14u, 41u, 42u, 13u, 4u, 15u, 37u, 48u, 22u, 9u, 27u, 17u, 21u, 20u, 49u, 44u, 23u, 6u, 12u, 35u, 29u, 11u, 33u, 25u, 30u, 50u, 26u, 1u, 8u, 45u, 19u, 16u, 3u, 28u, 51u, 39u, 31u, 34u, 46u, 43u, 38u, 40u, 7u, 2u, 24u, 32u, 0u, 18u, 10u),
   },
   round<cat::uint8>{
      .numbers = cat::array<cat::uint8, 6u>(0xaf64b2a7'30ffd1b9ull, 0xef9f2e94'6e08fbe3ull, 0x181b81b6'a0b5bee2ull, 0x219851c7'42250cf4ull, 0xb459875e'221e7df5ull, 0xe7518dd5'd411bae8ull),
      .coins = "HTHHTHTTTHHTTHHTTTTHTTHHTHTTHHHTTHTTHHHTTTHTTTTHHHHHTTTTTHHTHTTTT",
      .rolls = cat::array<cat::uint1, 33u>(1u, 3u, 6u, 6u, 2u, 1u, 1u, 4u, 2u, 5u, 1u, 3u, 1u, 5u, 1u, 5u, 1u, 6u, 2u, 3u, 1u, 2u, 3u, 4u, 2u, 4u, 5u, 4u, 2u, 2u, 4u, 5u, 2u),
      .cards = cat::array<cat::uint1, 52u>(41u, 40u, 7u, 11u, 1u, 19u, 38u, 49u, 27u, 36u, 26u, 16u, 10u, 3u, 46u, 12u, 8u, 43u, 13u, 18u, 37u, 14u, 31u, 0u, 20u, 17u, 5u, 25u, 9u, 47u, 23u, 4u, 51u, 39u, 33u, 6u, 50u, 21u, 28u, 44u, 35u, 2u, 34u, 15u, 42u, 29u, 24u, 22u, 45u, 30u, 48u, 32u),
   },
   round<cat::uint8>{
      .numbers = cat::array<cat::uint8, 6u>(0xe7e1d171'1485e473ull, 0xc0c6d1f0'e72a55d3ull, 0x2d0fa33e'b3638524ull, 0xd0cb8d73'a16deacdull, 0x3e410a9c'c7682918ull, 0x8df7d57b'4d2f9ac4ull),
      .coins = "THHTHTHTHHTHTHHTTHTHHTHHHTTTHTTHHHHTTTTHTTHTTHTTTTHTHHHTHTTHTHTTH",
      .rolls = cat::array<cat::uint1, 33u>(5u, 4u, 3u, 2u, 6u, 2u, 6u, 6u, 2u, 3u, 1u, 3u, 1u, 6u, 2u, 4u, 3u, 6u, 6u, 6u, 4u, 1u, 4u, 6u, 5u, 1u, 3u, 6u, 3u, 5u, 1u, 6u, 1u),
      .cards = cat::array<cat::uint1, 52u>(10u, 12u, 6u, 30u, 43u, 47u, 11u, 14u, 32u, 51u, 37u, 45u, 44u, 33u, 27u, 23u, 21u, 36u, 20u, 39u, 35u, 40u, 7u, 25u, 38u, 1u, 41u, 29u, 22u, 15u, 34u, 3u, 26u, 16u, 8u, 4u, 2u, 13u, 31u, 28u, 19u, 48u, 0u, 49u, 9u, 42u, 17u, 24u, 50u, 46u, 18u, 5u),
   },
   round<cat::uint8>{
      .numbers = cat::array<cat::uint8, 6u>(0x2a2d820a'8f859ba2ull, 0x1a74e59c'8e288526ull, 0x6b856b08'000af65cull, 0x793c0d41'03ce2a55ull, 0xc5081bea'922d1d0cull, 0x6b61da59'd73efbf9ull),
      .coins = "HTTHTHTTHTTTTTHTHTHHTHTHHHTHHHTTHHTHTTTTTHHHHHTHHHTTTHHTTTTHHHHHH",
      .rolls = cat::array<cat::uint1, 33u>(4u, 4u, 1u, 3u, 5u, 3u, 1u, 3u, 4u, 3u, 1u, 4u, 2u, 5u, 6u, 2u, 2u, 5u, 6u, 3u, 4u, 2u, 4u, 5u, 6u, 6u, 4u, 2u, 4u, 3u, 1u, 5u, 1u),
      .cards = cat::array<cat::uint1, 52u>(51u, 0u, 1u, 2u, 10u, 32u, 25u, 9u, 26u, 11u, 36u, 6u, 40u, 24u, 16u, 38u, 30u, 21u, 50u, 4u, 41u, 34u, 31u, 5u, 28u, 12u, 46u, 22u, 17u, 43u, 19u, 33u, 3u, 15u, 42u, 48u, 7u, 37u, 29u, 18u, 44u, 8u, 45u, 23u, 49u, 27u, 14u, 39u, 47u, 20u, 13u, 35u),
   },
};

}  // namespace pcg_high
