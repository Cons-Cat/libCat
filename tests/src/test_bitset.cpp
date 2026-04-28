#include <cat/bitset>

#include "../unit_tests.hpp"

test(bitset) {
   using namespace cat::arithmetic_literals;

   constexpr cat::bitset<7u> bits7{};
   constexpr cat::bitset<8> bits8{};
   constexpr cat::bitset<16u> bits16{};
   constexpr cat::bitset<17> bits17{};
   constexpr cat::bitset<32u> bits32{};
   constexpr cat::bitset<64> bits64{};
   constexpr cat::bitset<65u> bits65{};
   constexpr cat::bitset<129_u4> bits129{};

   static_assert(bits7.storage_array_size == 1);
   static_assert(bits8.storage_array_size == 1);
   static_assert(bits16.storage_array_size == 1);
   static_assert(bits17.storage_array_size == 1);
   static_assert(bits32.storage_array_size == 1);
   static_assert(bits64.storage_array_size == 1);
   static_assert(bits65.storage_array_size == 2);
   static_assert(bits129.storage_array_size == 3);

   static_assert(bits7.storage_element_size == 1);
   static_assert(bits8.storage_element_size == 1);
   static_assert(bits16.storage_element_size == 2);
   static_assert(bits17.storage_element_size == 4);
   static_assert(bits32.storage_element_size == 4);
   static_assert(bits64.storage_element_size == 8);
   static_assert(bits65.storage_element_size == 8);
   static_assert(bits129.storage_element_size == 8);

   static_assert(bits7.leading_skipped_bits == 1);
   static_assert(bits7.leading_bytes_bits == 7u);

   static_assert(sizeof(bits129) == 24u);
   static_assert(bits129.leading_skipped_bits == 63u);

   cat::bitset<7u> bits7_2 = bits7;
   bits7_2 = cat::make_bitset<7u>(0x0_u1);
   cat::verify(!bits7_2.all_of());
   cat::verify(bits7_2.none_of());
   cat::verify(!bits7_2.any_of());

   bits7_2 = cat::make_bitset<7u>(0b1111'1110_u1);
   cat::verify(bits7_2.all_of());
   cat::verify(!bits7_2.none_of());
   cat::verify(bits7_2.any_of());

   bits7_2 = cat::make_bitset<7u>(0b0100'0000_u1);
   cat::verify(!bits7_2.all_of());
   cat::verify(!bits7_2.none_of());
   cat::verify(bits7_2.any_of());

   bits7_2 = cat::make_bitset<7u>(0b1000'0000_u1);
   cat::verify(!bits7_2.all_of());
   cat::verify(!bits7_2.none_of());
   cat::verify(bits7_2.any_of());

   cat::bitset<127> bits127 = cat::make_bitset<127>(0x0_u8, 0x0_u8);
   cat::verify(!bits127.all_of());
   cat::verify(bits127.none_of());
   cat::verify(!bits127.any_of());

   // The 128th bit is off, `all_of` others are on. The lowest bit is off, which
   // is ignored by a 127-bit bitset.
   bits127 = cat::make_bitset<127>(cat::uint8_max << 1u, 0xFFFFFFFF'FFFFFFFEul);
   static_assert(bits127.leading_bytes_bits == 63u);
   static_assert(bits127.leading_skipped_bits == 1u);
   cat::verify(!bits127.all_of());
   cat::verify(!bits127.none_of());
   cat::verify(bits127.any_of());
   cat::verify(bits127.countl_zero() == 0u);
   cat::verify(bits127.countr_zero() == 0u);

   // The 128th bit is off, `all_of` others are on.
   bits127 =
      cat::make_bitset<127>(cat::uint8_max >> 2u, 0xFFFFFFFF'FFFFFFFF_u8 << 1u);
   cat::verify(bits127.countl_zero() == 2u);
   cat::verify(bits127.countr_zero() == 0u);

   bits127 = cat::make_bitset<127>(0_u8, cat::uint8_max >> 1u);
   cat::verify(bits127.countl_zero() == 65u);
   cat::verify(bits127.countr_zero() == 0u);

   // Test `const` subscripting.
   constexpr cat::bitset<15u> bits15 =
      cat::make_bitset<15u>(0b0101'0101'0101'0100_u2);
   static_assert(!bits15[0u]);
   static_assert(bits15[1]);
   static_assert(!bits15[2u]);

   constexpr cat::bitset<15u> bits15_2 =
      cat::make_bitset<15u>(0b1010'1010'1010'1010_u2);
   static_assert(bits15_2[0u]);
   static_assert(!bits15_2[1]);
   static_assert(bits15_2[2]);

   // Test 16-byte bitset's subscript.
   constexpr cat::bitset<128> bits128_2 =
      cat::make_bitset<128>(0xFFFFFFFF'FFFFFFFF_u8, 0xFFFFFFFF'FFFFFFFB_u8);
   // 11111111'11111111'11111111'11111111'11111111'11111111'11111111'11111011.
   static_assert(bits128_2[0u]);
   static_assert(bits128_2[1u]);
   static_assert(!bits128_2[2u]);
   static_assert(bits128_2[125u]);
   static_assert(bits128_2[126u]);
   static_assert(bits128_2[127u]);

   // Test 16-byte bitset's subscript with bit offset.
   constexpr cat::bitset<127> bits127_2 =
      cat::make_bitset<127>(0xFFFFFFFF'FFFFFFFF_u8, 0xFFFFFFFF'FFFFFFFB_u8);
   // 11111111'11111111'11111111'11111111'11111111'11111111'11111111'1111101.
   static_assert(bits127_2[0u]);
   static_assert(!bits127_2[1u]);
   static_assert(bits127_2[2u]);
   static_assert(bits127_2[125u]);
   static_assert(bits127_2[126u]);

   // Test mutable subscript.
   bits127 = cat::make_bitset<127>(cat::uint8_max >> 2u, 0b0000'0100_u8);

   cat::verify(!bits127[0u]);
   cat::verify(bits127[1u]);

   bits127[1u] = false;
   cat::verify(!bits127[1u]);
   bits127[1u] = true;
   cat::verify(bits127[1u]);

   bits127[0u] = true;
   cat::verify(bits127[0u]);
   bits127[0u] = false;
   cat::verify(!bits127[0u]);

   // Test this on the second element of uint8 array. `bitset` is zero-indexed,
   // so the largest addressable bit is 126. The left-most two bits are unset,
   // so 126 and 125 should be unset, but 124 should be set.
   cat::verify(!bits127[125u]);
   cat::verify(bits127[124u]);
   bits127[124u] = false;
   cat::verify(!bits127[124u]);
   bits127[124u] = true;
   cat::verify(bits127[124u]);

   // Test `const` `.at()`.
   auto _ = bits127_2.at(0u).verify();
   cat::verify(!bits127_2.at(128u).has_value());

   // Test mutable `.at()`.
   bits127.at(0u).verify() = true;
   cat::verify(bits127.at(0u).has_value());
   cat::verify(!bits127.at(128u).has_value());

   // Test `const` iterator.
   for ([[maybe_unused]]
        auto bit : bits127_2) {
   }

   // Test mutable 8-byte aligned iterator.
   cat::bitset bits128 =
      cat::make_bitset<128>(cat::uint8_max >> 2u, 0b0000'0100_u8);
   for (cat::bit_reference bit : bits128) {
      bit = false;
   }
   for (cat::bit_reference bit : bits128) {
      cat::verify(bit == false);
   }

   for (cat::bit_reference bit : bits128) {
      bit = true;
   }
   for (cat::bit_reference bit : bits128) {
      cat::verify(bit == true);
   }

   auto bits127_3 = cat::make_bitset<127>(cat::uint8_min, 0b0000'0100_u8);
   // Test mutable non-8-byte aligned iterator.
   for (cat::bit_reference bit : bits127_3) {
      bit = false;
   }
   for (cat::bit_reference bit : bits127_3) {
      cat::verify(bit == false);
   }

   for (cat::bit_reference bit : bits127_3) {
      bit = true;
   }
   for (cat::bit_reference bit : bits127_3) {
      cat::verify(bit == true);
   }

   // TODO: Get this working.
   //  for (cat::bit_reference bit : cat::as_reverse_stepanov(bits127_3)) {
   //     cat::verify(bit == true);
   //  }

   cat::bitset bitstring("010101");
   cat::verify(bitstring[0] == false);
   cat::verify(bitstring[1] == true);
   cat::verify(bitstring[2] == false);
   cat::verify(bitstring[3] == true);
   cat::verify(bitstring[4] == false);
   cat::verify(bitstring[5] == true);

   cat::bitset fullbits = cat::make_bitset_filled<8>(true);
   cat::verify(fullbits.all_of());
   cat::bitset nonebits = cat::make_bitset_filled<8>(false);
   cat::verify(nonebits.none_of());

   // Test `.rotate_left()` and `.rotate_right()` using the bitstring
   // constructor so logical index `i` corresponds directly to character `i`.
   constexpr cat::bitset<15u> bits15_3("100000000000000");
   static_assert(bits15_3[0u]);
   static_assert(!bits15_3[1u]);
   static_assert(!bits15_3[14u]);

   // Rotating by 0 or `bits_count` is the identity.
   cat::verify(bits15_3.rotate_left(0).rotate_right(0) == bits15_3);
   cat::verify(bits15_3.rotate_left(15) == bits15_3);
   cat::verify(bits15_3.rotate_right(15) == bits15_3);
   cat::verify(bits15_3.rotate_left(30) == bits15_3);

   // Rotating left by 1 moves bit 0 to bit 1.
   cat::bitset<15u> bits15_3_rotl1 = bits15_3.rotate_left(1);
   cat::verify(!bits15_3_rotl1[0u]);
   cat::verify(bits15_3_rotl1[1u]);
   cat::verify(!bits15_3_rotl1[14u]);

   // Rotating left by 14 wraps bit 0 to the top.
   cat::bitset<15u> bits15_3_rotl14 = bits15_3.rotate_left(14);
   cat::verify(!bits15_3_rotl14[0u]);
   cat::verify(bits15_3_rotl14[14u]);

   // Rotating right undoes rotating left.
   cat::verify(bits15_3.rotate_left(3).rotate_right(3) == bits15_3);
   cat::verify(bits15_3.rotate_left(7).rotate_right(7) == bits15_3);

   // Negative shifts rotate the opposite direction.
   cat::verify(bits15_3.rotate_left(-1) == bits15_3.rotate_right(1));
   cat::verify(bits15_3.rotate_right(-4) == bits15_3.rotate_left(4));

   // Test the fast path that delegates to `cat::rotate_left` /
   // `cat::rotate_right`: `bitset<8>` fully fills its storage (no padding
   // bits).
   constexpr cat::bitset<8u> bits8_rot = cat::make_bitset<8u>(0b0000'0001_u1);
   cat::verify(bits8_rot[0u]);
   cat::verify(bits8_rot.rotate_left(1)[1u]);
   cat::verify(bits8_rot.rotate_left(7)[7u]);
   cat::verify(bits8_rot.rotate_right(1)[7u]);

   // Rotate a multi-word bitset. Set only bit 66. Rotating right by 66 should
   // move that bit to position 0.
   cat::bitset<127> bits127_4;
   bits127_4[66u] = true;
   cat::verify(bits127_4[66u]);
   cat::bitset<127> bits127_4_rotr66 = bits127_4.rotate_right(66);
   cat::verify(bits127_4_rotr66[0u]);
   cat::verify(!bits127_4_rotr66[66u]);

   // Rotating by `bits_count` returns to the original.
   cat::verify(bits127_4.rotate_left(127) == bits127_4);
   cat::verify(bits127_4.rotate_right(127) == bits127_4);

   // Test `.set_even()`, `.set_odd()`, `.unset_even()`, `.unset_odd()`, and the
   // `make_bitset_even()`/`make_bitset_odd()` factories.

   // An even-bits bitset sets logical indices 0, 2, 4, ... to 1 and all
   // odd-index bits to 0.
   constexpr cat::bitset<8u> even8 = cat::make_bitset_even<8u>();
   static_assert(even8[0u]);
   static_assert(!even8[1u]);
   static_assert(even8[2u]);
   static_assert(!even8[3u]);
   static_assert(even8[6u]);
   static_assert(!even8[7u]);

   constexpr cat::bitset<8u> odd8 = cat::make_bitset_odd<8u>();
   static_assert(!odd8[0u]);
   static_assert(odd8[1u]);
   static_assert(!odd8[2u]);
   static_assert(odd8[7u]);

   // Even and odd are complementary and together fill the bitset.
   cat::verify((even8 | odd8).all_of());
   cat::verify((even8 & odd8).none_of());

   // Exercise the padded single-word case: `bitset<7>` has a one-bit LSB
   // padding, so logical index 0 is at an odd physical bit.
   cat::bitset<7u> even7 = cat::make_bitset_even<7u>();
   cat::verify(even7[0u]);
   cat::verify(!even7[1u]);
   cat::verify(even7[2u]);
   cat::verify(!even7[3u]);
   cat::verify(even7[4u]);
   cat::verify(!even7[5u]);
   cat::verify(even7[6u]);

   cat::bitset<7u> odd7 = cat::make_bitset_odd<7u>();
   cat::verify(!odd7[0u]);
   cat::verify(odd7[1u]);
   cat::verify(!odd7[6u]);

   // The padding bit must still be zero after `set_*`.
   cat::verify(even7.countl_zero() == 0u);
   cat::verify(odd7.countl_zero() == 1u);

   // `set_*` composes: `set_even().set_odd()` fills every usable bit.
   cat::bitset<17u> all17;
   all17.set_even().set_odd();
   for (cat::idx i = 0u; i < 17u; ++i) {
      cat::verify(all17[i]);
   }

   // From an all-set bitset, `unset_even` leaves `make_bitset_odd` and
   // `unset_odd` leaves `make_bitset_even`.
   cat::bitset<17u> unset_even_17 = all17;
   unset_even_17.unset_even();
   cat::verify(unset_even_17 == cat::make_bitset_odd<17u>());

   cat::bitset<17u> unset_odd_17 = all17;
   unset_odd_17.unset_odd();
   cat::verify(unset_odd_17 == cat::make_bitset_even<17u>());

   // Multi-word case.
   cat::bitset<129u> even129 = cat::make_bitset_even<129u>();
   cat::verify(even129[0u]);
   cat::verify(!even129[1u]);
   cat::verify(!even129[63u]);
   cat::verify(even129[64u]);
   cat::verify(even129[128u]);

   cat::bitset<129u> odd129 = cat::make_bitset_odd<129u>();
   cat::verify(!odd129[0u]);
   cat::verify(odd129[1u]);
   cat::verify(!odd129[128u]);

   // Together, `make_bitset_even` and `make_bitset_odd` cover every bit.
   cat::bitset<129u> together129;
   together129.set_even().set_odd();
   for (cat::idx i = 0u; i < 129u; ++i) {
      cat::verify(together129[i]);
   }

   // Test bitwise operators generated by `bitwise_interface`, and the explicit
   // `operator^`/`operator^=` overloads.

   // ~ must complement every usable bit and preserve the padding invariant.
   cat::bitset<17u> complement_zero_17 = ~cat::bitset<17u>{};
   for (cat::idx i = 0u; i < 17u; ++i) {
      cat::verify(complement_zero_17[i]);
   }
   cat::verify(complement_zero_17.countl_zero() == 0u);

   // ~ is involutive on a bitset that doesn't touch padding.
   cat::bitset<17u> roundtrip_17 = ~complement_zero_17;
   cat::verify(roundtrip_17 == cat::bitset<17u>{});

   // ^ of two equal-valued bitsets is all zero. ^ With a zero bitset is the
   // identity.
   cat::bitset<17u> const all_ones_17_a = ~cat::bitset<17u>{};
   cat::bitset<17u> const all_ones_17_b = ~cat::bitset<17u>{};
   cat::verify((all_ones_17_a ^ all_ones_17_b).none_of());
   cat::verify((complement_zero_17 ^ cat::bitset<17u>{}) == complement_zero_17);

   // ^= assigns in place.
   cat::bitset<17u> xor_accum = complement_zero_17;
   xor_accum ^= all_ones_17_a;
   cat::verify(xor_accum.none_of());

   // &= / |= compound assignment (generated from `bit_and`/`bit_or`).
   cat::bitset<17u> and_accum = complement_zero_17;
   and_accum &= cat::make_bitset_even<17u>();
   cat::verify(and_accum == cat::make_bitset_even<17u>());

   cat::bitset<17u> or_accum = cat::make_bitset_even<17u>();
   or_accum |= cat::make_bitset_odd<17u>();
   cat::verify(or_accum == complement_zero_17);

   // Test `.clear()` and `.fill()`.

   // `.fill()` on a default-constructed bitset sets every usable bit to 1 and
   // preserves the zero-padding invariant.
   cat::bitset<17u> fill17;
   fill17.fill();
   cat::verify(fill17.all_of());
   cat::verify(!fill17.none_of());
   cat::verify(fill17.countl_zero() == 0u);
   cat::verify(fill17 == complement_zero_17);

   // `.clear()` zeroes out every bit.
   cat::bitset<17u> clear_me = complement_zero_17;
   clear_me.clear();
   cat::verify(clear_me.none_of());
   cat::verify(clear_me == cat::bitset<17u>{});

   // `.fill(false)` is equivalent to `.clear()`.
   cat::bitset<17u> fill_false_17 = complement_zero_17;
   fill_false_17.fill(false);
   cat::verify(fill_false_17.none_of());
   cat::verify(fill_false_17 == cat::bitset<17u>{});

   // Both return a reference so they chain.
   cat::bitset<17u> chained;
   chained.fill().clear().fill();
   cat::verify(chained.all_of());

   // Word-aligned single-word case.
   cat::bitset<8u> fill8;
   fill8.fill();
   cat::verify(fill8.all_of());
   fill8.clear();
   cat::verify(fill8.none_of());

   // Multi-word case preserves padding on the top word.
   cat::bitset<129u> fill129;
   fill129.fill();
   cat::verify(fill129.all_of());
   cat::verify(fill129.countl_zero() == 0u);
   fill129.clear();
   cat::verify(fill129.none_of());
}
