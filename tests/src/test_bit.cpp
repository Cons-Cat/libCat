#include <cat/bit>

#include "../unit_tests.hpp"

TEST(test_bit) {
   using namespace cat::arithmetic_literals;

   // Test clz().
   static_assert(cat::countl_zero(0x7FFFFFFF'FFFFFFFFu) == 1);
   static_assert(cat::countl_zero(0x7FFFFFFFu) == 1);
   static_assert(cat::countl_one(0x7FFFFFFF'FFFFFFFFu) == 0);
   static_assert(cat::countl_one(0x7FFFFFFFu) == 0);

   static_assert(cat::countl_zero(0xFFFFFFFF_u4) == 0);
   static_assert(cat::countl_zero(0xFFFFFFFF'FFFFFFFF_u8) == 0);

   static_assert(cat::countl_zero(0x0FFFFFFF_u4) == 4);
   static_assert(cat::countl_zero(0x0FFFFFFF'FFFFFFFF_u8) == 4);

   // Test ctz().
   static_assert(cat::countr_zero(0xFFFFFFFF'FFFFFFFEu) == 1);
   static_assert(cat::countr_zero(0xFFFFFFFEu) == 1);
   static_assert(cat::countr_one(0xFFFFFFFF'FFFFFFFEu) == 0);
   static_assert(cat::countr_one(0xFFFFFFFEu) == 0);

   // Test small integers.
   static_assert(cat::countl_zero(0x0F_u1) == 4);
   cat::verify(cat::countl_zero(0x0F_u1) == 4);
   static_assert(cat::countr_zero(0x0F_u1) == 0);
   cat::verify(cat::countr_zero(0x0F_u1) == 0);

   static_assert(cat::countr_zero(0xF0_u1) == 4);
   cat::verify(cat::countr_zero(0xF0_u1) == 4);

   static_assert(cat::countl_zero(0x0FFF_u2) == 4);
   cat::verify(cat::countl_zero(0x0FFF_u2) == 4);
   static_assert(cat::countr_zero(0x0FFF_u2) == 0);
   cat::verify(cat::countr_zero(0x0FFF_u2) == 0);

   static_assert(cat::countr_zero(0xFFF0_u2) == 4);
   cat::verify(cat::countr_zero(0xFFF0_u2) == 4);

   // Test popcnt.
   static_assert(cat::popcount(0b01'0101'0011u) == 5);
   static_assert(cat::popcount(0b0'1010'1001ull) == 4);
   static_assert(cat::popcount(0b0'1010'1001_u8) == 4);
   static_assert(cat::popcount(0b0'1010'1011_u1) == 5);
   static_assert(cat::popcount(0b0'1010'1011_u2) == 5);

   // Test bextr.
   static_assert(x64::extract_bits(uint1::max() >> 1, 4_u1, 4u) == 0b0111u);
   static_assert(x64::extract_bits(uint1::max(), 4_u1, 4u) == 0b1111u);
   static_assert(x64::extract_bits(uint1::max() >> 1, 0_u1, 4u) == 0b1111u);
   static_assert(x64::extract_bits(uint1::max() >> 1, 0_u1, 5u) == 0b1'1111u);

   static_assert(x64::extract_bits(uint4::max() >> 1, 27_u1, 4u) == 0b1111u);
   static_assert(x64::extract_bits(uint4::max() >> 1, 28_u1, 4u) == 0b0111u);

   static_assert(x64::extract_bits(uint8::max() >> 1, 59_u1, 4u) == 0b1111u);
   static_assert(x64::extract_bits(uint8::max() >> 1, 60_u1, 4u) == 0b0111u);

   // Test pext.
   static_assert(x64::parallel_extract_bits(uint8::max(), 0b1ull) == 0b1ull);
   static_assert(x64::parallel_extract_bits(uint4::max(), 0b1u) == 0b1u);
   static_assert(x64::parallel_extract_bits(uint2::max(), 0b1_u2) == 0b1_u2);

   // Test bzhi.
   // TODO: These only test it compiles. Test that it works correctly.
   cat::assert(x64::zero_high_bits_at(8_u4, 8u) != 0u);
   cat::assert(x64::zero_high_bits_at(8_u8, 8u) != 0u);

   // Test `bit_value`.
   cat::bit_value bit1 = false;
   bit1 = true;
   cat::verify(bit1.is_set());
   cat::verify(~bit1 == false);
   cat::verify(~~bit1 == true);
   cat::verify(false == ~bit1);
   // NOLINTNEXTLINE
   cat::verify(bit1 == bit1);

   // Test `bit_reference`
   unsigned char number = 0u;
   cat::bit_reference bit2 =
      cat::bit_reference<unsigned char>::from_mask(number, 0b0000'0010u);
   bit2 = true;
   cat::verify(bit2.is_set());
   cat::verify(bit2 == true);
   cat::verify(bit2);
   cat::verify(number != 0u);

   cat::bit_reference bit3 = bit1;
   cat::verify(bit3 == true);
   bit3.unset();
   cat::verify(bit3 == false);

   cat::bit_reference bit4 =
      cat::bit_reference<unsigned char>::from_offset(number, 1u);
   cat::bit_reference bit5 =
      cat::bit_reference<unsigned char>::from_offset(number, 5u);
   cat::verify(bit4.is_set());
   cat::verify(!bit5.is_set());
   number = 0;
   cat::verify(!bit4.is_set());
   cat::verify(!bit5.is_set());
   number = 1 << 5;
   cat::verify(!bit4.is_set());
   cat::verify(bit5.is_set());

   // Assign a `bit_reference` to a `bit_value`.
   number = 0b10;
   bit1 = bit2;
   cat::verify(bit1);
   bit1 = bit5;
   cat::verify(!bit1);

   // Test `bit_iterator`.
   cat::array<uint4, 4u> array(0u, cat::uint4_max, 0u, 0u);
   cat::bit_iterator it(array.begin());

   // 1st bit of 1st uint4 is 0:
   cat::verify(*it == false);

   // 32nd bit of 1st uint4 is 0:
   cat::verify(*(it + 31) == false);

   // 1st bit of 2nd uint4 is 1:
   cat::verify(*(it + 32) == true);

   // 2nd bit of 2nd uint4 is 1:
   it += 33u;
   cat::verify(*it == true);

   // The next 30 bits are 1:
   for (int4 i = 0; i < 30; ++i) {
      ++it;
      cat::verify(*it == true);
   }

   // 1st bit of 3rd uint4 is 0:
   it++;
   cat::verify(*it == false);
}
