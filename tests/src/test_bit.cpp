#include <cat/bit>
#include <cat/iterable>
#include <cat/page_allocator>
#include <cat/span>
#include <cat/vec>

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

struct proxy_bit_popcount_context {
   cat::uint1* m_p_storage = nullptr;

   using element_type = cat::bit_reference<cat::uint1>;

   template <typename Predicate>
   constexpr auto
   run_while(Predicate&& predicate) -> cat::iteration_result {
      for (cat::uword bit_offset = 0u; bit_offset < 8u; ++bit_offset) {
         if (!predicate(
                cat::make_bit_reference_from_offset(*m_p_storage, bit_offset)
             )) {
            return cat::iteration_result::incomplete;
         }
      }
      return cat::iteration_result::complete;
   }
};

struct proxy_bit_popcount_source : cat::iterable_interface<> {
   cat::uint1 m_storage = 0u;

   constexpr explicit proxy_bit_popcount_source(cat::uint1 storage)
       : m_storage(storage) {
   }

   constexpr auto
   iterate() -> proxy_bit_popcount_context {
      return {&m_storage};
   }
};
}  // namespace

$test(bit_constraints_and_alignment) {
   static_assert(can_align_pointer_to_constant<32u>);
   static_assert(!can_align_pointer_to_constant<12u>);
   static_assert(can_align_intptr_to_constant<32u>);
   static_assert(!can_align_intptr_to_constant<12u>);
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
   static_assert(cat::bit_width(cat::byte(5u)) == 3u);
   static_assert(cat::byteswap(cat::byte(0xA5u)) == 0xA5u);
   static_assert(cat::shift_left(cat::byte(1u), 8u) == 0u);
   static_assert(cat::shift_right(cat::byte(0x80u), 7u) == 1u);
   static_assert(cat::bit_reverse(cat::byte(0b0001'0110u)) == 0b0110'1000u);
   static_assert(cat::bit_repeat(cat::byte(0b1110u), 2u) == 0b1010'1010u);
   static_assert(
      cat::bit_compress(cat::byte(0b0100'1001u), cat::byte(0b1100'1100u))
      == 0b0000'0110u
   );
   static_assert(
      cat::bit_expand(cat::byte(0b0000'0110u), cat::byte(0b1100'1100u))
      == 0b0100'1000u
   );
   static_assert(
      cat::rotate_left(cat::byte(0b1000'0001u), cat::uword(1u)) == 0b0000'0011u
   );
   static_assert(
      cat::rotate_right(cat::byte(0b1000'0001u), cat::uword(1u)) == 0b1100'0000u
   );
   static_assert(cat::countl_zero_raw(cat::byte(0x0Fu)).value() == 4u);
   static_assert(!cat::countl_zero_raw(cat::byte(0u)).has_value());

   auto const byte_bit_ceil = cat::bit_ceil_raw(cat::byte(5u));
   cat::verify(byte_bit_ceil.has_value());
   cat::verify(byte_bit_ceil.value() == 8u);

   cat::byte byte_value(0b0001'0110u);
   cat::verify(byte_value.bit_width() == 5u);
   cat::verify(byte_value.bit_reverse() == 0b0110'1000u);
   cat::verify(
      byte_value.bit_compress(cat::byte(0b1100'1100u)) == 0b0000'0001u
   );
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
   static_assert(cat::bit_cast<unsigned>(2_i4) == 2u);
   static_assert(cat::byteswap(0x1234_u2) == 0x3412_u2);
   static_assert(cat::bit_width(0u) == 0u);
   static_assert(cat::bit_width(0b1000u) == 4u);
   static_assert(cat::shift_left(1_u1, 7u) == 0b1000'0000_u1);
   static_assert(cat::shift_left(1_u1, 8u) == 0_u1);
   static_assert(cat::shift_right(0b1000'0000_u1, 7u) == 1_u1);
   static_assert(cat::shift_right(0b1000'0000_u1, 8u) == 0_u1);
   static_assert(cat::bit_reverse(0b0001'0110_u1) == 0b0110'1000_u1);
   static_assert(cat::bit_repeat(0b1110_u1, 2u) == 0b1010'1010_u1);
   static_assert(
      cat::bit_compress(0b0100'1001_u1, 0b1100'1100_u1) == 0b0000'0110_u1
   );
   static_assert(
      cat::bit_expand(0b0000'0110_u1, 0b1100'1100_u1) == 0b0100'1000_u1
   );
   static_assert(cat::endian::native == cat::endian::little);
   static_assert([] {
      cat::array<cat::byte, 4u> bytes{
         cat::byte(0b1010'1010u), cat::byte(0b1111'0000u),
         cat::byte(0b0000'0000u), cat::byte(0b1111'1111u)
      };
      return cat::popcount(cat::span<cat::byte const>(bytes)) == 16u;
   }());
   cat::array<cat::byte, 5u> bytes{
      cat::byte(0b1111'1111u), cat::byte(0b0000'1111u), cat::byte(0b1010'1010u),
      cat::byte(0u), cat::byte(0b1000'0001u)
   };
   cat::verify(cat::popcount(cat::span<cat::byte const>(bytes)) == 18u);

   cat::array<cat::uword, 3u> words{
      cat::uword(0b1111u), cat::uword(0b1010'1010u), cat::uword(0b1111'0000u)
   };
   cat::verify(cat::popcount(cat::span<cat::uword const>(words)) == 12u);

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
      == cat::limits<cat::idx>::high_bit
   );
   cat::verify(
      !cat::bit_ceil_raw(cat::idx(cat::limits<cat::idx>::high_bit + 1u))
          .has_value()
   );
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

   static_assert(
      cat::rotate_left(0b1000'0001_u1, cat::uword(1u)) == 0b0000'0011_u1
   );
   static_assert(
      cat::rotate_right(0b1000'0001_u1, cat::uword(1u)) == 0b1100'0000_u1
   );
   static_assert(
      cat::rotate_left(0b1000'0001_u1, cat::uword(9u))
      == cat::rotate_left(0b1000'0001_u1, cat::uword(1u))
   );
   cat::verify(
      cat::rotate_left(0b1000'0001_u1, cat::uword(9u))
      == cat::rotate_left(0b1000'0001_u1, cat::uword(1u))
   );
   cat::verify(
      cat::rotate_right(0b1000'0001_u1, cat::uword(9u))
      == cat::rotate_right(0b1000'0001_u1, cat::uword(1u))
   );
   static_assert(
      cat::rotate_left(0b1000'0001_u1, 6)
      == cat::rotate_left(0b1000'0001_u1, cat::uword(6u))
   );
   static_assert(
      cat::rotate_right(0b1000'0001_u1, 6)
      == cat::rotate_right(0b1000'0001_u1, cat::uword(6u))
   );
   static_assert(
      cat::rotate_left(0b1000'0001_u1, cat::iword(-1))
      == cat::rotate_right(0b1000'0001_u1, cat::uword(1u))
   );
   static_assert(
      cat::rotate_right(0b1000'0001_u1, cat::iword(-1))
      == cat::rotate_left(0b1000'0001_u1, cat::uword(1u))
   );
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

$test(bit_reference_popcount_terminal) {
   using namespace cat::arithmetic_literals;

   static_assert(cat::is_iterable<proxy_bit_popcount_source>);

   proxy_bit_popcount_source bits{0b1010'1100_u1};
   cat::verify((bits | cat::popcount()) == 4u);
}

$test(bit_word_iterable_popcount_terminal) {
   using namespace cat::arithmetic_literals;

   cat::page_allocator pager;
   cat::vec<cat::uint1> words;
   words.resize(pager, 2u, 0_u1).verify();
   words[0u] = 0b1010'1100_u1;
   words[1u] = 0b0000'0011_u1;

   cat::verify((words | cat::popcount()) == 6u);
   cat::verify(words.popcount() == 6u);
   cat::verify(cat::popcount(words) == 6u);
   words.free(pager);

   cat::vec<cat::uword> uword_words;
   uword_words.resize(pager, 2u, 0u).verify();
   uword_words[0u] = 0b1010'1100_u1;
   uword_words[1u] = 0b0000'0011_u1;

   cat::verify(cat::popcount(uword_words) == 6u);
   cat::verify((uword_words | cat::popcount()) == 6u);
   uword_words.free(pager);
}

$test(bit_ptr) {
   cat::bit_ptr<unsigned char> null_bit = nullptr;
   cat::verify(null_bit == nullptr);
   cat::verify(!null_bit);
   unsigned char bit_words[2]{};
   cat::bit_ptr bit_pointer = cat::make_bit_ptr_from_offset(bit_words, 1u);
   *bit_pointer = true;
   cat::verify(bit_words[0] == 0b0000'0010u);
   cat::verify(
      cat::make_bit_ptr_from_mask(bit_words, 0b0000'0010u) == bit_pointer
   );
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
