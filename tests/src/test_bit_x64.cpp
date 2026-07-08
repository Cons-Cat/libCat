#include <cat/bit>

#include "../unit_tests.hpp"

$test(bit_x64_helpers) {
   using namespace cat::arithmetic_literals;

   static_assert(x64::extract_bits(uint1::max() >> 1, 4_u1, 4u) == 0b0111u);
   static_assert(x64::extract_bits(uint1::max(), 4_u1, 4u) == 0b1111u);
   static_assert(x64::extract_bits(uint1::max() >> 1, 0_u1, 4u) == 0b1111u);
   static_assert(x64::extract_bits(uint1::max() >> 1, 0_u1, 5u) == 0b1'1111u);

   static_assert(x64::extract_bits(uint4::max() >> 1, 27_u1, 4u) == 0b1111u);
   static_assert(x64::extract_bits(uint4::max() >> 1, 28_u1, 4u) == 0b0111u);

   static_assert(x64::extract_bits(uint8::max() >> 1, 59_u1, 4u) == 0b1111u);
   static_assert(x64::extract_bits(uint8::max() >> 1, 60_u1, 4u) == 0b0111u);

   static_assert(cat::bit_compress(uint8::max(), uint8(0b1u)) == 0b1u);
   static_assert(cat::bit_compress(uint4::max(), 0b1_u4) == 0b1u);
   static_assert(cat::bit_compress(uint2::max(), 0b1_u2) == 0b1_u2);
   static_assert(cat::bit_compress(0b1101_u1, 0b0101_u1) == 0b11_u1);
   static_assert(cat::bit_expand(0b11_u1, 0b1010_u1) == 0b1010_u1);

   static_assert(x64::zero_high_bits_at(0b1111'0000_u1, 4u) == 0u);
   static_assert(x64::zero_high_bits_at(0b1111'0000_u1, 8u) == 0b1111'0000_u1);
   cat::verify(x64::zero_high_bits_at(8_u4, 8u) == 8_u4);
   cat::verify(x64::zero_high_bits_at(8_u8, 8u) == 8_u8);

   static_assert(x64::and_not(0b1100_u1, 0b1010_u1) == 0b0010_u1);
   static_assert(x64::and_not(0xFFFF0000_u4, 0xF0F0F0F0_u4) == 0x0000F0F0_u4);
   cat::verify(
      x64::and_not(0xFFFF0000'00000000_u8, uint8::max())
      == 0x0000FFFF'FFFFFFFF_u8
   );

   static_assert(x64::extract_lowest_bit(0b1010'1000_u1) == 0b0000'1000_u1);
   static_assert(x64::extract_lowest_bit(0_u4) == 0_u4);
   static_assert(
      x64::mask_through_lowest_bit(0b1010'1000_u1) == 0b0000'1111_u1
   );
   static_assert(x64::mask_through_lowest_bit(0_u1) == uint1::max());
   static_assert(x64::reset_lowest_bit(0b1010'1000_u1) == 0b1010'0000_u1);
   static_assert(x64::reset_lowest_bit(0_u8) == 0_u8);
   cat::verify(x64::reset_lowest_bit(0b1000_u4) == 0_u4);

   static_assert(cat::shift_left(0b0000'0001_u1, 3u) == 0b0000'1000_u1);
   static_assert(cat::shift_left(1_u4, 32u) == 0_u4);
   static_assert(cat::shift_right(0b1000'0000_u1, 7u) == 1_u1);
   static_assert(cat::shift_right(1_u8, 64u) == 0_u8);
   static_assert(x64::shift_right_arithmetic_by(-8_i4, 2u) == -2_i4);
   static_assert(x64::shift_right_arithmetic_by(-8_i4, 32u) == -1_i4);
   static_assert(
      cat::rotate_right(0b1000'0001_u1, cat::uword(1u)) == 0b1100'0000_u1
   );
   static_assert(
      cat::rotate_right(0b1000'0001_u1, cat::uword(9u))
      == cat::rotate_right(0b1000'0001_u1, cat::uword(1u))
   );
   cat::verify(cat::shift_left(1_u8, 4u) == 16_u8);
   cat::verify(cat::shift_right(16_u8, 4u) == 1_u8);
   cat::verify(
      cat::rotate_right(0b0000'0011_u1, cat::uword(1u)) == 0b1000'0001_u1
   );
}
