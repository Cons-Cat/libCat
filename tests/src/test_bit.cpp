#include <cat/bit>

#include "../unit_tests.hpp"

namespace {
template <typename T>
concept can_countl_zero = requires(T value) { cat::countl_zero(value); };

template <typename T>
concept can_countl_zero_raw =
   requires(T value) { cat::countl_zero_raw(value); };

template <typename T>
concept can_countl_one = requires(T value) { cat::countl_one(value); };

template <typename T>
concept can_countl_one_raw = requires(T value) { cat::countl_one_raw(value); };

template <typename T>
concept can_countr_zero = requires(T value) { cat::countr_zero(value); };

template <typename T>
concept can_countr_zero_raw =
   requires(T value) { cat::countr_zero_raw(value); };

template <typename T>
concept can_countr_one = requires(T value) { cat::countr_one(value); };

template <typename T>
concept can_countr_one_raw = requires(T value) { cat::countr_one_raw(value); };

template <typename T>
concept can_popcount = requires(T value) { cat::popcount(value); };

template <typename T>
concept can_has_single_bit = requires(T value) { cat::has_single_bit(value); };

template <typename T>
concept can_bit_floor = requires(T value) { cat::bit_floor(value); };

template <typename T>
concept can_bit_ceil = requires(T value) { cat::bit_ceil(value); };

template <typename T>
concept can_bit_ceil_raw = requires(T value) { cat::bit_ceil_raw(value); };

template <typename T>
concept can_rotate_left =
   requires(T value) { cat::rotate_left(value, cat::iword(1)); };

template <typename T>
concept can_rotate_right =
   requires(T value) { cat::rotate_right(value, cat::iword(1)); };
}  // namespace

test(bit) {
   using namespace cat::arithmetic_literals;

   static_assert(!can_countl_zero<cat::idx>);
   static_assert(!can_countl_zero_raw<cat::idx>);
   static_assert(!can_countl_one<cat::idx>);
   static_assert(!can_countl_one_raw<cat::idx>);
   static_assert(can_countr_zero<cat::idx>);
   static_assert(can_countr_zero_raw<cat::idx>);
   static_assert(can_countr_one<cat::idx>);
   static_assert(can_countr_one_raw<cat::idx>);
   static_assert(can_popcount<cat::idx>);
   static_assert(!can_has_single_bit<cat::idx>);
   static_assert(!can_rotate_left<cat::idx>);
   static_assert(!can_rotate_right<cat::idx>);

   // Test `clz()`.
   static_assert(cat::countl_zero(0_u4) == 32u);
   static_assert(cat::countl_zero(0_u8) == 64u);
   static_assert(cat::countl_zero(0x7FFFFFFF'FFFFFFFFu) == 1);
   static_assert(cat::countl_zero_raw(0x7FFFFFFF'FFFFFFFFu).value() == 1);
   static_assert(cat::countl_zero(0x7FFFFFFFu) == 1);
   static_assert(cat::countl_zero_raw(0x7FFFFFFFu).value() == 1);
   auto raw_countl_runtime = 0x7FFFFFFF_u4;
   cat::verify(cat::countl_zero_raw(raw_countl_runtime).value() == 1);
   raw_countl_runtime = 0_u4;
   cat::verify(!cat::countl_zero_raw(raw_countl_runtime).has_value());
   static_assert(cat::countl_one(0x7FFFFFFF'FFFFFFFFu) == 0);
   static_assert(cat::countl_one_raw(0x7FFFFFFF'FFFFFFFFu).value() == 0);
   static_assert(cat::countl_one(0x7FFFFFFFu) == 0);
   static_assert(cat::countl_one_raw(0x7FFFFFFFu).value() == 0);
   auto raw_countl_one_runtime = 0x7FFFFFFF_u4;
   cat::verify(cat::countl_one_raw(raw_countl_one_runtime).value() == 0);
   raw_countl_one_runtime = 0_u4;
   cat::verify(!cat::countl_one_raw(raw_countl_one_runtime).has_value());

   static_assert(cat::countl_zero(0xFFFFFFFF_u4) == 0);
   static_assert(cat::countl_zero(0xFFFFFFFF'FFFFFFFF_u8) == 0);

   static_assert(cat::countl_zero(0x0FFFFFFF_u4) == 4);
   static_assert(cat::countl_zero(0x0FFFFFFF'FFFFFFFF_u8) == 4);

   // Test `ctz()`.
   static_assert(cat::countr_zero(0_u4) == 32u);
   static_assert(cat::countr_zero(0_u8) == 64u);
   static_assert(cat::countr_zero(cat::idx{0u}) == __SIZE_WIDTH__);
   static_assert(cat::countr_zero(cat::idx{8u}) == 3u);
   static_assert(cat::countr_zero_raw(cat::idx{8u}).value() == 3u);
   auto raw_countr_runtime = cat::idx{8u};
   cat::verify(cat::countr_zero_raw(raw_countr_runtime).value() == 3u);
   raw_countr_runtime = cat::idx{0u};
   cat::verify(!cat::countr_zero_raw(raw_countr_runtime).has_value());
   static_assert(cat::countr_one(cat::idx{7u}) == 3u);
   static_assert(cat::countr_one_raw(cat::idx{7u}).value() == 3u);
   auto raw_countr_one_runtime = cat::idx{7u};
   cat::verify(cat::countr_one_raw(raw_countr_one_runtime).value() == 3u);
   raw_countr_one_runtime = cat::idx{0u};
   cat::verify(!cat::countr_one_raw(raw_countr_one_runtime).has_value());
   static_assert(cat::countr_zero(0xFFFFFFFF'FFFFFFFEu) == 1);
   static_assert(cat::countr_zero_raw(0xFFFFFFFF'FFFFFFFEu).value() == 1);
   static_assert(cat::countr_zero(0xFFFFFFFEu) == 1);
   static_assert(cat::countr_zero_raw(0xFFFFFFFEu).value() == 1);
   static_assert(cat::countr_one(0xFFFFFFFF'FFFFFFFEu) == 0);
   static_assert(cat::countr_one_raw(0xFFFFFFFF'FFFFFFFEu).value() == 0);
   static_assert(cat::countr_one(0xFFFFFFFEu) == 0);
   static_assert(cat::countr_one_raw(0xFFFFFFFEu).value() == 0);

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
   static_assert(cat::popcount(cat::idx{0b0'1010'1011u}) == 5);
   static_assert(cat::popcount(0_u4) == 0);

   // Test `has_single_bit()`. Matches C23 `stdc_has_single_bit`, so zero is not
   // a single-bit value.
   cat::verify(!cat::has_single_bit(0));
   cat::verify(cat::has_single_bit(1));
   cat::verify(cat::has_single_bit(2));
   cat::verify(!cat::has_single_bit(3));
   cat::verify(cat::has_single_bit(4));
   cat::verify(cat::has_single_bit(8u));
   cat::verify(cat::has_single_bit(256uz));

   // Test `bit_ceil()`.
   static_assert(cat::bit_ceil(0u) == 1u);
   static_assert(cat::bit_ceil(1u) == 1u);
   static_assert(cat::bit_ceil(2u) == 2u);
   static_assert(cat::bit_ceil(3u) == 4u);
   static_assert(cat::bit_ceil(4u) == 4u);
   static_assert(cat::bit_ceil(5u) == 8u);
   cat::verify(cat::bit_ceil_raw(3u).value() == 4u);
   cat::verify(cat::bit_ceil_raw(5u).value() == 8u);
   cat::verify(cat::bit_ceil_raw(0u).value() == 1u);
   cat::verify(cat::bit_ceil_raw(cat::idx(5u)).value() == 8u);
   cat::verify(!cat::bit_ceil_raw(0x80000001u).has_value());
   cat::verify(
      cat::bit_ceil_raw(cat::idx(cat::limits<cat::idx>::high_bit)).value()
      == cat::limits<cat::idx>::high_bit);
   cat::verify(
      !cat::bit_ceil_raw(cat::idx(cat::limits<cat::idx>::high_bit + 1u))
          .has_value());
   auto raw_bit_ceil_runtime = 0x80000001_u4;
   cat::verify(!cat::bit_ceil_raw(raw_bit_ceil_runtime).has_value());

   // Test `bit_floor()`.
   static_assert(cat::bit_floor(0u) == 0u);
   static_assert(cat::bit_floor(1u) == 1u);
   static_assert(cat::bit_floor(2u) == 2u);
   static_assert(cat::bit_floor(3u) == 2u);
   static_assert(cat::bit_floor(4u) == 4u);
   static_assert(cat::bit_floor(5u) == 4u);

   // Test rotate.
   static_assert(cat::rotate_left(0b1000'0001_u1, cat::uword(1u))
                 == 0b0000'0011_u1);
   static_assert(cat::rotate_right(0b1000'0001_u1, cat::uword(1u))
                 == 0b1100'0000_u1);
   static_assert(cat::rotate_left(0b1000'0001_u1, cat::uword(9u))
                 == cat::rotate_left(0b1000'0001_u1, cat::uword(1u)));
   cat::verify(cat::rotate_left(0b1000'0001_u1, cat::uword(9u))
               == cat::rotate_left(0b1000'0001_u1, cat::uword(1u)));
   cat::verify(cat::rotate_right(0b1000'0001_u1, cat::uword(9u))
               == cat::rotate_right(0b1000'0001_u1, cat::uword(1u)));
   static_assert(cat::rotate_left(0b1000'0001_u1, 6)
                 == cat::rotate_left(0b1000'0001_u1, cat::uword(6u)));
   static_assert(cat::rotate_right(0b1000'0001_u1, 6)
                 == cat::rotate_right(0b1000'0001_u1, cat::uword(6u)));
   static_assert(cat::rotate_left(0b1000'0001_u1, cat::iword(-1))
                 == cat::rotate_right(0b1000'0001_u1, cat::uword(1u)));
   static_assert(cat::rotate_right(0b1000'0001_u1, cat::iword(-1))
                 == cat::rotate_left(0b1000'0001_u1, cat::uword(1u)));

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

   // Test `bit_reference`.
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

   // Test `bit_stepanov_iterator`.
   cat::array<uint4, 4u> array(0u, cat::uint4_max, 0u, 0u);
   cat::bit_stepanov_iterator it(array.begin());

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
