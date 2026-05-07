#include <cat/bitset>
#include <cat/iterable>

#include "../unit_tests.hpp"

$test(bitset_storage_layout) {
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
}

// `__datasizeof` is `sizeof` minus tail padding. `cat::bitset<N>` should always
// pack tightly to its storage backing array with no tail padding, so the two
// must agree at every bit count. This guards against a regression where the
// CRTP `bitwise_interface` base or the storage `cat::array` member introduces
// padding that would put `cat::bitset` out of step with the C ABI it mirrors
// (e.g. compiler-rt's `__cpu_features2[N]` layout).
$test(bitset_layout_data_size) {
   // Single-word storage. The element type widens to the smallest unsigned
   // integer that fits the bit count.
   static_assert(__datasizeof(cat::bitset<1>) == 1u);
   static_assert(__datasizeof(cat::bitset<7>) == 1u);
   static_assert(__datasizeof(cat::bitset<8>) == 1u);
   static_assert(__datasizeof(cat::bitset<9>) == 2u);
   static_assert(__datasizeof(cat::bitset<15>) == 2u);
   static_assert(__datasizeof(cat::bitset<16>) == 2u);
   static_assert(__datasizeof(cat::bitset<17>) == 4u);
   static_assert(__datasizeof(cat::bitset<31>) == 4u);
   static_assert(__datasizeof(cat::bitset<32>) == 4u);
   static_assert(__datasizeof(cat::bitset<33>) == 8u);
   static_assert(__datasizeof(cat::bitset<63>) == 8u);
   static_assert(__datasizeof(cat::bitset<64>) == 8u);

   // Multi-word storage uses 8-byte words.
   static_assert(__datasizeof(cat::bitset<65>) == 16u);
   static_assert(__datasizeof(cat::bitset<128>) == 16u);
   static_assert(__datasizeof(cat::bitset<129>) == 24u);
   static_assert(__datasizeof(cat::bitset<192>) == 24u);
   static_assert(__datasizeof(cat::bitset<256>) == 32u);

   // No tail padding: `sizeof` always equals `__datasizeof`.
   static_assert(sizeof(cat::bitset<1>) == __datasizeof(cat::bitset<1>));
   static_assert(sizeof(cat::bitset<7>) == __datasizeof(cat::bitset<7>));
   static_assert(sizeof(cat::bitset<32>) == __datasizeof(cat::bitset<32>));
   static_assert(sizeof(cat::bitset<33>) == __datasizeof(cat::bitset<33>));
   static_assert(sizeof(cat::bitset<128>) == __datasizeof(cat::bitset<128>));
   static_assert(sizeof(cat::bitset<129>) == __datasizeof(cat::bitset<129>));
}

$test(bitset_make_and_predicates) {
   using namespace cat::arithmetic_literals;

   constexpr cat::bitset<7u> bits7{};
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
}

$test(bitset_count_leading_trailing_zero) {
   using namespace cat::arithmetic_literals;

   // The 128th bit is off, all others are on. The lowest bit is off, which is
   // ignored by a 127-bit bitset.
   cat::bitset<127> bits127 =
      cat::make_bitset<127>(cat::uint8_max << 1u, 0xFFFFFFFF'FFFFFFFEul);
   static_assert(bits127.leading_bytes_bits == 63u);
   static_assert(bits127.leading_skipped_bits == 1u);
   cat::verify(!bits127.all_of());
   cat::verify(!bits127.none_of());
   cat::verify(bits127.any_of());
   cat::verify(bits127.countl_zero() == 0u);
   cat::verify(bits127.countr_zero() == 0u);

   // The 128th bit is off, all others are on.
   bits127 =
      cat::make_bitset<127>(cat::uint8_max >> 2u, 0xFFFFFFFF'FFFFFFFF_u8 << 1u);
   cat::verify(bits127.countl_zero() == 2u);
   cat::verify(bits127.countr_zero() == 0u);

   bits127 = cat::make_bitset<127>(0_u8, cat::uint8_max >> 1u);
   cat::verify(bits127.countl_zero() == 65u);
   cat::verify(bits127.countr_zero() == 0u);
}

$test(bitset_subscript_const) {
   using namespace cat::arithmetic_literals;

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
}

$test(bitset_subscript_multiword) {
   using namespace cat::arithmetic_literals;

   // 16-byte bitset's subscript.
   constexpr cat::bitset<128> bits128_2 =
      cat::make_bitset<128>(0xFFFFFFFF'FFFFFFFF_u8, 0xFFFFFFFF'FFFFFFFB_u8);
   // 11111111'11111111'11111111'11111111'11111111'11111111'11111111'11111011.
   static_assert(bits128_2[0u]);
   static_assert(bits128_2[1u]);
   static_assert(!bits128_2[2u]);
   static_assert(bits128_2[125u]);
   static_assert(bits128_2[126u]);
   static_assert(bits128_2[127u]);

   // 16-byte bitset's subscript with bit offset.
   constexpr cat::bitset<127> bits127_2 =
      cat::make_bitset<127>(0xFFFFFFFF'FFFFFFFF_u8, 0xFFFFFFFF'FFFFFFFB_u8);
   // 11111111'11111111'11111111'11111111'11111111'11111111'11111111'1111101.
   static_assert(bits127_2[0u]);
   static_assert(!bits127_2[1u]);
   static_assert(bits127_2[2u]);
   static_assert(bits127_2[125u]);
   static_assert(bits127_2[126u]);
}

$test(bitset_subscript_mutable) {
   using namespace cat::arithmetic_literals;

   cat::bitset<127> bits127 =
      cat::make_bitset<127>(cat::uint8_max >> 2u, 0b0000'0100_u8);

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
}

$test(bitset_swap) {
   using namespace cat::arithmetic_literals;

   cat::bitset<8u> swap_left = cat::make_bitset<8u>(0b1010'0000_u1);
   cat::bitset<8u> swap_right = cat::make_bitset<8u>(0b0101'0000_u1);
   cat::swap(swap_left, swap_right);
   cat::verify(swap_left == cat::make_bitset<8u>(0b0101'0000_u1));
   cat::verify(swap_right == cat::make_bitset<8u>(0b1010'0000_u1));
}

$test(bitset_at) {
   using namespace cat::arithmetic_literals;

   constexpr cat::bitset<127> bits127_2 =
      cat::make_bitset<127>(0xFFFFFFFF'FFFFFFFF_u8, 0xFFFFFFFF'FFFFFFFB_u8);

   // `const` `.at()`.
   auto _ = bits127_2.at(0u).verify();
   cat::verify(!bits127_2.at(128u).has_value());

   // Mutable `.at()`.
   cat::bitset<127> bits127 =
      cat::make_bitset<127>(cat::uint8_max >> 2u, 0b0000'0100_u8);
   bits127.at(0u).verify() = true;
   cat::verify(bits127.at(0u).has_value());
   cat::verify(!bits127.at(128u).has_value());
}

$test(bitset_from_string) {
   cat::bitset bitstring("010101");
   cat::verify(bitstring[0] == false);
   cat::verify(bitstring[1] == true);
   cat::verify(bitstring[2] == false);
   cat::verify(bitstring[3] == true);
   cat::verify(bitstring[4] == false);
   cat::verify(bitstring[5] == true);
}

$test(bitset_make_filled) {
   cat::bitset fullbits = cat::make_bitset_filled<8>(true);
   cat::verify(fullbits.all_of());
   cat::bitset nonebits = cat::make_bitset_filled<8>(false);
   cat::verify(nonebits.none_of());
}

$test(bitset_rotate) {
   using namespace cat::arithmetic_literals;

   // The bitstring constructor maps logical index `i` directly to character
   // `i`, so this test layout is straightforward to read.
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
}

$test(bitset_set_even_odd) {
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
}

$test(bitset_unset_even_odd_compose) {
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

   // Together, `make_bitset_even` and `make_bitset_odd` cover every bit in
   // the multi-word case.
   cat::bitset<129u> together129;
   together129.set_even().set_odd();
   for (cat::idx i = 0u; i < 129u; ++i) {
      cat::verify(together129[i]);
   }
}

$test(bitset_bitwise_operators) {
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
}

$test(bitset_clear_fill) {
   cat::bitset<17u> const complement_zero_17 = ~cat::bitset<17u>{};

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

$test(bitset_stepanov_iterator) {
   using namespace cat::arithmetic_literals;

   constexpr cat::bitset<127> bits127_2 =
      cat::make_bitset<127>(0xFFFFFFFF'FFFFFFFF_u8, 0xFFFFFFFF'FFFFFFFB_u8);

   for ([[maybe_unused]]
        auto bit : bits127_2) {
   }

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
}

$test(bitset_collection) {
   static_assert(cat::is_random_access_collection<cat::bitset<9u>>);
   cat::bitset<5u> bits;
   bits[0u] = true;
   bits[2u] = true;

   cat::verify((bits | cat::count()) == 5u);
   cat::verify(bool(cat::read_at(bits, 0u)));
   cat::verify(!bool(cat::read_at(bits, 1u)));
   cat::verify(bool(cat::read_at(bits, 2u)));

   idx true_count = 0u;
   bits | cat::for_each([&true_count](auto bit) {
      if (bit) {
         ++true_count;
      }
   });
   cat::verify(true_count == 2u);
   auto true_bit_values = cat::ref(bits)
                             .filter([](auto bit) -> bool {
                                return bit;
                             })
                             .transform([](auto bit) -> int {
                                return bit ? 10 : 0;
                             });
   cat::verify(true_bit_values.sum() == 20);

   auto reversed = cat::reverse_iterate(bits);
   auto first = cat::next_element(reversed);
   cat::verify(first.has_value());
   cat::verify(!first.value());
}
