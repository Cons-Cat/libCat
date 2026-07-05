#include <cat/bit>

#include "../unit_tests.hpp"

namespace {
template <auto alignment>
concept can_align_pointer_to_constant = requires(int* p_value) {
                                           cat::align_down(p_value, alignment);
                                           cat::align_up(p_value, alignment);
                                           cat::is_aligned(p_value, alignment);
                                        };

template <auto alignment>
concept can_align_intptr_to_constant = requires(cat::uintptr<int> p_value) {
                                          cat::align_down(p_value, alignment);
                                          cat::align_up(p_value, alignment);
                                          cat::is_aligned(p_value, alignment);
                                       };

template <auto alignment>
concept can_assume_aligned_to_constant =
   requires(int* p_value) { cat::assume_aligned<alignment>(p_value); };
}  // namespace

$test(bit_constraints_and_alignment) {
   static_assert(can_align_pointer_to_constant<32u>);
   static_assert(!can_align_pointer_to_constant<12u>);
   static_assert(can_align_intptr_to_constant<32u>);
   static_assert(!can_align_intptr_to_constant<12u>);
   static_assert(can_assume_aligned_to_constant<32u>);
   static_assert(can_assume_aligned_to_constant<32>);
   static_assert(!can_assume_aligned_to_constant<12u>);
   static_assert(!can_assume_aligned_to_constant<12>);
   static_assert(!can_assume_aligned_to_constant<-32>);
   alignas(32) int aligned_value = 0;
   cat::verify(cat::assume_aligned<32u>(&aligned_value) == &aligned_value);
   cat::verify(cat::assume_aligned<32>(&aligned_value) == &aligned_value);
   int* p_null_int = nullptr;
   cat::verify(cat::align_down(p_null_int, 32u) == nullptr);
   cat::verify(cat::align_up(p_null_int, 32u) == nullptr);
   cat::verify(cat::is_aligned(p_null_int, 32u));
}

$test(bit_count_leading) {
   using namespace cat::arithmetic_literals;

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
}

$test(bit_count_trailing) {
   using namespace cat::arithmetic_literals;

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
}

$test(bit_count_small_integer_wrappers) {
   using namespace cat::arithmetic_literals;

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
}

$test(byte_bitwise_api) {
   static_assert(!cat::is_addable<cat::byte>);
   static_assert(!cat::is_subtractable<cat::byte>);
   static_assert(!cat::is_multipliable<cat::byte>);
   static_assert(!cat::is_dividable<cat::byte>);
   static_assert(!cat::is_modable<cat::byte>);
   static_assert(!cat::is_unary_plussable<cat::byte>);
   static_assert(!cat::is_unary_minusable<cat::byte>);
   static_assert(cat::is_constructible<char, cat::byte>);
   static_assert(cat::is_constructible<signed char, cat::byte>);
   static_assert(cat::is_constructible<unsigned char, cat::byte>);
   static_assert(cat::is_constructible<char8_t, cat::byte>);
   static_assert(!cat::is_constructible<bool, cat::byte>);

   static_assert((cat::byte(1u) <=> cat::byte(2u)) < 0);
   static_assert((cat::byte(2u) <=> 1u) > 0);
   static_assert((1u <=> cat::byte(2u)) < 0);
   static_assert((cat::byte(1u) <=> -1) > 0);
   static_assert((-1 <=> cat::byte(1u)) < 0);

   static_assert((cat::byte(0b1010u) & cat::byte(0b1100u)) == 0b1000u);
   static_assert((cat::byte(0b1010u) | 0b0101u) == 0b1111u);
   static_assert((0b0101u | cat::byte(0b1010u)) == 0b1111u);
   static_assert((cat::byte(0b1010u) ^ cat::byte(0b1111u)) == 0b0101u);
   static_assert((~cat::byte(0xF0u)) == 0x0Fu);
   static_assert((cat::byte(0b0000'1111u) << 4u) == 0b1111'0000u);
   static_assert((cat::byte(0b1111'0000u) >> cat::byte(4u)) == 0b0000'1111u);

   static_assert([] {
      cat::byte value = cat::byte(0b1010u);
      value &= cat::byte(0b1100u);
      value |= 0b0011u;
      value ^= cat::byte(0b0001u);
      value <<= 1u;
      value >>= cat::byte(1u);
      return value == 0b1010u;
   }());

   static_assert(cat::countl_zero(cat::byte(0x0Fu)) == 4u);
   static_assert(cat::countl_one(cat::byte(0xF0u)) == 4u);
   static_assert(cat::countr_zero(cat::byte(0xF0u)) == 4u);
   static_assert(cat::countr_one(cat::byte(0x0Fu)) == 4u);
   static_assert(cat::popcount(cat::byte(0b1010'1010u)) == 4u);
   static_assert(cat::has_single_bit(cat::byte(0b0001'0000u)));
   static_assert(cat::bit_floor(cat::byte(5u)) == 4u);
   static_assert(cat::bit_ceil(cat::byte(5u)) == 8u);
   static_assert(cat::rotate_left(cat::byte(0b1000'0001u), cat::uword(1u))
                 == 0b0000'0011u);
   static_assert(cat::rotate_right(cat::byte(0b1000'0001u), cat::uword(1u))
                 == 0b1100'0000u);
   static_assert(cat::countl_zero_raw(cat::byte(0x0Fu)).value() == 4u);
   static_assert(!cat::countl_zero_raw(cat::byte(0u)).has_value());

   auto const byte_bit_ceil = cat::bit_ceil_raw(cat::byte(5u));
   cat::verify(byte_bit_ceil.has_value());
   cat::verify(byte_bit_ceil.value() == 8u);
}

$test(bit_popcount_and_single_bit) {
   using namespace cat::arithmetic_literals;

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
}

$test(bit_floor_and_ceil) {
   using namespace cat::arithmetic_literals;

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
}

$test(bit_rotate) {
   using namespace cat::arithmetic_literals;

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
}

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

   // Test pext.
   static_assert(x64::parallel_extract_bits(uint8::max(), 0b1ull) == 0b1ull);
   static_assert(x64::parallel_extract_bits(uint4::max(), 0b1u) == 0b1u);
   static_assert(x64::parallel_extract_bits(uint2::max(), 0b1_u2) == 0b1_u2);
   static_assert(x64::parallel_extract_bits(0b1101_u1, 0b0101_u1) == 0b11_u1);
   static_assert(x64::deposit_bits_mask(0b11_u1, 0b1010_u1) == 0b1010_u1);

   static_assert(x64::zero_high_bits_at(0b1111'0000_u1, 4u) == 0u);
   static_assert(x64::zero_high_bits_at(0b1111'0000_u1, 8u) == 0b1111'0000_u1);
   cat::verify(x64::zero_high_bits_at(8_u4, 8u) == 8_u4);
   cat::verify(x64::zero_high_bits_at(8_u8, 8u) == 8_u8);
}

$test(bit_value) {
   cat::bit_value bit1 = false;
   bit1 = true;
   cat::verify(bit1.is_set());
   cat::verify(~bit1 == false);
   cat::verify(~~bit1 == true);
   cat::verify(false == ~bit1);
   // NOLINTNEXTLINE
   cat::verify(bit1 == bit1);
}

$test(bit_reference) {
   cat::bit_value bit1 = true;
   unsigned char number = 0u;
   cat::bit_reference bit2 =
      cat::make_bit_reference_from_mask(number, 0b0000'0010u);
   bit2 = true;
   cat::verify(bit2.is_set());
   cat::verify(bit2 == true);
   cat::verify(bit2);
   cat::verify(number != 0u);

   cat::bit_reference bit3 = bit1;
   cat::verify(bit3 == true);
   bit3.unset();
   cat::verify(bit3 == false);

   cat::bit_reference bit4 = cat::make_bit_reference_from_offset(number, 1u);
   cat::bit_reference bit5 = cat::make_bit_reference_from_offset(number, 5u);
   cat::verify(bit4.is_set());
   cat::verify(!bit5.is_set());
   number = 0;
   cat::verify(!bit4.is_set());
   cat::verify(!bit5.is_set());
   number = 1 << 5;
   cat::verify(!bit4.is_set());
   cat::verify(bit5.is_set());
   unsigned char rebound_number = 0u;
   bit4.rebind(rebound_number, 0b0000'0001u);
   bit4.set();
   cat::verify(rebound_number == 0b0000'0001u);
   cat::verify(number == 0b0010'0000u);

   // Assign a `bit_reference` to a `bit_value`.
   number = 0b10;
   bit1 = bit2;
   cat::verify(bit1);
   bit1 = bit5;
   cat::verify(!bit1);
}

$test(bit_ptr) {
   cat::bit_ptr<unsigned char> null_bit = nullptr;
   cat::verify(null_bit == nullptr);
   cat::verify(!null_bit);
   unsigned char bit_words[2]{};
   cat::bit_ptr bit_pointer = cat::make_bit_ptr_from_offset(bit_words, 1u);
   *bit_pointer = true;
   cat::verify(bit_words[0] == 0b0000'0010u);
   cat::verify(cat::make_bit_ptr_from_mask(bit_words, 0b0000'0010u)
               == bit_pointer);
   bit_pointer[7] = true;
   cat::verify(bit_words[1] == 0b0000'0001u);
   cat::bit_ptr next_word_bit = bit_pointer + 8;
   cat::verify(next_word_bit.storage() == bit_words + 1u);
   cat::verify(next_word_bit.bit_position() == 1u);
   *next_word_bit = true;
   cat::verify(bit_words[1] == 0b0000'0011u);
   cat::verify(next_word_bit - bit_pointer == 8);
   cat::verify(bit_pointer - next_word_bit == -8);
   --next_word_bit;
   cat::verify(next_word_bit == bit_pointer + 7);
   cat::verify(next_word_bit > bit_pointer);
   cat::verify(bit_pointer < next_word_bit);
   next_word_bit -= 7;
   cat::verify(next_word_bit == bit_pointer);
}

$test(bit_stepanov_iterator) {
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
