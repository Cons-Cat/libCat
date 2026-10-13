#include <cat/hash>
#include <cat/simd>

#include "../unit_tests.hpp"

namespace {

// Byte `i` is `i * 37 + 11`, and the seed for a length is
// `length * 0x9e3779b97f4a7c15`. Expected values are from the reference
// `wyhash.h`.
consteval auto
wyhash_matches(idx length, uint8 expected) -> bool {
   cat::array<cat::byte, 200u> bytes{};
   for (idx i = 0u; i < bytes.size(); ++i) {
      bytes[i] = cat::byte((i * 37u + 11u).raw);
   }
   uint8 const seed = (uint8(length) * 0x9e3779b9'7f4a7c15ull).wrap();
   return cat::wyhash(cat::span<cat::byte const>(bytes.data(), length), seed)
          == expected;
}

}  // namespace

$test(wyhash_bytes) {
   // Lengths at each boundary between the reference's code paths.
   static_assert(wyhash_matches(0u, 0x93228a4d'e0eec5a2ull));
   static_assert(wyhash_matches(1u, 0x4772b7ea'0a984982ull));
   static_assert(wyhash_matches(3u, 0x0109cc65'c1a63bc1ull));
   static_assert(wyhash_matches(4u, 0x11bc2dbe'cc6df429ull));
   static_assert(wyhash_matches(8u, 0x4fd1efcf'ba6ab1faull));
   static_assert(wyhash_matches(16u, 0x403057f7'92fe1daaull));
   static_assert(wyhash_matches(17u, 0x49f78bfe'ff4ebe26ull));
   static_assert(wyhash_matches(32u, 0xe15342a6'bdb3e130ull));
   static_assert(wyhash_matches(47u, 0x1c11719a'8cdc20efull));
   static_assert(wyhash_matches(48u, 0x2040df06'0accbfc5ull));
   static_assert(wyhash_matches(49u, 0x566dbc87'74821457ull));
   static_assert(wyhash_matches(96u, 0x14fe116f'c84bc47bull));
   static_assert(wyhash_matches(200u, 0x71d5d645'26e2576cull));
}

$test(wyhash_integers) {
   // The seed for case `i` is `i * 7`.
   static_assert(cat::wyhash(int8(0), 0u) == 0xfa303abc'2b1d7630ull);
   static_assert(cat::wyhash(int8(1), 7u) == 0xb0c8d1b8'f6959866ull);
   static_assert(cat::wyhash(int8(-1), 14u) == 0xf6aec7fd'65d8cd17ull);
   static_assert(cat::wyhash(int8(42), 21u) == 0x37cbbcca'4cbdfe4full);
   static_assert(
      cat::wyhash(int8(0x7fffffff'ffffffffll), 28u) == 0xe0430ef8'3d29ba8full
   );
   static_assert(
      cat::wyhash(uint8(0x80000000'00000000ull), 35u) == 0x3b4909a9'78201b9eull
   );
   static_assert(cat::wyhash(int8(123'456'789), 42u) == 0x089130a7'c6f727e5ull);
}

$test(wyhash_simd) {
   cat::fixed_size_simd<uint4, 4u> const value{1u, 2u, 3u, 4u};
   auto const seeded = cat::wyhash(value, 99u);
   auto const unseeded = cat::hash(value);
   using seeded_type = decltype(seeded);

   static_assert(cat::is_simd<seeded_type>);
   static_assert(cat::is_same<seeded_type::value_type, uint8>);
   static_assert(seeded_type::abi_type::lanes == 4u);

   for (idx lane = 0u; lane < seeded_type::abi_type::lanes; ++lane) {
      auto const lane_value = value[lane];
      cat::verify(seeded[lane] == cat::wyhash(lane_value, 99u));
      cat::verify(unseeded[lane] == cat::wyhash(lane_value));
   }
}
