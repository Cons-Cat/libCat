#include <cat/bit>
#include <cat/bitset>
#include <cat/bitvec>
#include <cat/iterable>
#include <cat/page_allocator>
#include <cat/raii_bitvec>
#include <cat/span>
#include <cat/vec>

#include "../unit_tests.hpp"

$test(bit_span_popcount) {
   cat::array<cat::uword, 2u> constexpr_words{
      0b1111_u8, cat::uword(0b1010'1111u)
   };
   cat::verify(cat::popcount(cat::bit_span(constexpr_words, 0u, 68u)) == 8u);

   cat::array<cat::uword, 3u> words{
      cat::uword(0b1111u), cat::uword(0b1010'1010u), cat::uword(0b1111'0000u)
   };
   cat::verify(cat::bit_span(words, 0u, 132u).popcount() == 8u);
}

$test(bit_span) {
   using mutable_bit_span = cat::bit_span<cat::uint4>;
   static_assert(cat::is_same<mutable_bit_span::storage_type, cat::uint4>);
   static_assert(cat::is_same<mutable_bit_span::value_type, cat::bit_value>);
   static_assert(
      cat::is_same<mutable_bit_span::const_value_type, cat::bit_value const>
   );
   static_assert(
      cat::is_same<mutable_bit_span::reference, cat::bit_reference<cat::uint4>>
   );
   static_assert(cat::is_same<
                 mutable_bit_span::const_reference,
                 cat::bit_reference<cat::uint4 const>>);
   static_assert(
      cat::is_same<mutable_bit_span::pointer, cat::bit_ptr<cat::uint4>>
   );
   static_assert(
      cat::is_same<
         mutable_bit_span::const_pointer, cat::bit_ptr<cat::uint4 const>>
   );
   static_assert(cat::is_same<mutable_bit_span::size_type, cat::idx>);
   static_assert(cat::is_same<mutable_bit_span::difference_type, cat::iword>);
   static_assert(cat::is_same<
                 mutable_bit_span::iterator,
                 cat::bit_stepanov_iterator<mutable_bit_span::word_iterator>>);
   static_assert(
      cat::is_same<
         mutable_bit_span::const_iterator,
         cat::bit_stepanov_iterator<mutable_bit_span::const_word_iterator>>
   );
   static_assert(
      cat::is_same<
         mutable_bit_span::reverse_iterator,
         cat::reverse_proxy_stepanov_iterator<mutable_bit_span::iterator>>
   );
   static_assert(
      cat::is_same<
         mutable_bit_span::const_reverse_iterator,
         cat::reverse_proxy_stepanov_iterator<mutable_bit_span::const_iterator>>
   );
   using bit_span_context =
      cat::iterable_iteration_context_type<mutable_bit_span>;
   static_assert(
      cat::is_same<bit_span_context::element_type, mutable_bit_span::reference>
   );

   cat::array<cat::uint4, 2u> array_words{0b1010_u4, 0u};
   cat::bit_span array_bits(array_words);
   cat::verify(array_bits.size() == 64u);
   cat::verify(!array_bits[0u]);
   cat::verify(array_bits[1u]);
   array_bits[0u] = true;
   cat::verify(array_words[0u] == 0b1011_u4);
   cat::verify(cat::popcount(array_bits) == 3u);
   cat::verify(array_bits.popcount() == 3u);
   cat::verify(!array_bits.has_single_bit());
   cat::verify(array_bits.countl_one() == 2u);
   cat::verify(array_bits.countl_zero() == 0u);
   cat::verify(array_bits.countr_zero() == 60u);
   cat::verify(cat::bit_width(array_bits) == 64u);
   cat::verify(array_bits.count() == 64u);
   cat::verify(
      array_bits
         .filter([](auto bit) -> bool {
            return bit;
         })
         .count()
      == 3u
   );
   cat::idx reverse_weight = 1u;
   cat::verify(
      (array_bits
       | cat::reverse()
       | cat::transform([&reverse_weight](auto bit) -> cat::idx {
            cat::idx const weight = reverse_weight;
            ++reverse_weight;
            return bit ? weight : 0u;
         })
       | cat::sum())
      == 188u
   );
   auto inplace_result = array_bits | cat::reverse_inplace();
   static_cast<void>(inplace_result);
   cat::verify(array_bits[60u]);
   cat::verify(array_bits[62u]);
   cat::verify(array_bits[63u]);
   array_bits.reverse_inplace();
   cat::verify(array_bits[0u]);
   auto array_bit_slice = cat::slice(array_bits, 0u, 4u);
   cat::verify(
      array_bit_slice
         .filter([](auto bit) -> bool {
            return bit;
         })
         .count()
      == 3u
   );
   cat::read_at(array_bit_slice, 2u) = true;
   cat::verify(array_bits[2u]);
   cat::verify(
      array_bit_slice
         .transform([](auto bit) -> cat::idx {
            return bit ? 1u : 0u;
         })
         .sum()
      == 4u
   );
   cat::idx array_range_count = 0u;
   for (cat::bit_reference bit : array_bits) {
      if (bit) {
         ++array_range_count;
      }
   }
   cat::verify(array_range_count == 4u);
   cat::verify(*array_bits.cbegin() == true);
   cat::verify(*(array_bits.cend() - 1u) == false);
   cat::verify(*array_bits.rbegin() == false);
   cat::verify(*(array_bits.rend() - 1u) == true);
   cat::verify(*array_bits.crbegin() == false);
   cat::verify(*(array_bits.crend() - 1u) == true);

   cat::span array_word_span(array_words);
   cat::bit_span span_bits(array_word_span);
   cat::verify(span_bits.size() == 64u);
   cat::verify(span_bits[0u]);
   cat::span<cat::uint4 const> const_array_word_span(array_words);
   cat::bit_span const_span_bits(const_array_word_span);
   cat::verify(const_span_bits.size() == 64u);
   cat::verify(const_span_bits[1u]);

   cat::vec<cat::uint4> vec_words;
   vec_words.resize(pager, 2u, 0_u4).verify();
   vec_words[1u] = 0b11_u4;
   cat::bit_span vec_bits(vec_words);
   cat::verify(vec_bits.size() == 64u);
   cat::verify(vec_bits[32u]);
   cat::verify(cat::popcount(vec_bits) == 2u);
   vec_words.free(pager);

   cat::raii::bitvec dynamic_bits{cat::dyn_allocator(pager)};
   dynamic_bits.resize(70u).verify();
   dynamic_bits[69u] = true;
   cat::bit_span dynamic_span(dynamic_bits);
   cat::verify(dynamic_span.size() == 70u);
   cat::verify(dynamic_span[69u]);
   cat::verify(cat::popcount(dynamic_span) == 1u);

   cat::bitset<7u> fixed_bits = cat::make_bitset<7u>(0b1111'1110_u1);
   cat::bit_span fixed_span(fixed_bits);
   cat::verify(fixed_span.size() == 7u);
   cat::verify(fixed_span[0u]);
   cat::verify(cat::popcount(fixed_span) == 7u);
   cat::verify((fixed_span | cat::count()) == 7u);
   cat::verify((fixed_span | cat::popcount()) == 7u);
}

$test(bit_span_algorithms) {
   cat::array<cat::uint1, 1u> left_words{0b1011'0000_u1};
   cat::array<cat::uint1, 1u> right_words{0u};
   cat::bit_span left(left_words, 0u, 8u);
   cat::bit_span right(right_words, 0u, 8u);

   right.copy_from(left);
   cat::verify(right.equal(left));
   cat::verify(cat::popcount(right) == 3u);

   right.shift_left(4u);
   cat::verify(right_words[0u] == 0b0000'1011_u1);
   right.shift_right(2u);
   cat::verify(right_words[0u] == 0b0010'1100_u1);

   right.reverse_inplace();
   cat::verify(right_words[0u] == 0b0011'0100_u1);

   cat::array<cat::uint1, 1u> same_word_storage{0u};
   cat::bit_span same_word_bits(same_word_storage, 1u, 6u);
   bool const same_word_values[6] = {true, false, true, true, false, false};
   for (cat::idx bit_index = 0u; bit_index < 6u; ++bit_index) {
      same_word_bits[bit_index] = same_word_values[bit_index];
   }
   same_word_bits.reverse_inplace();
   for (cat::idx bit_index = 0u; bit_index < 6u; ++bit_index) {
      cat::verify(
         same_word_bits[bit_index] == same_word_values[5u - bit_index]
      );
   }

   cat::array<cat::uint1, 2u> cross_word_storage{0u, 0u};
   cat::bit_span cross_word_bits(cross_word_storage, 3u, 10u);
   bool const cross_word_values[10] = {
      true, false, false, true, true, false, true, false, true, true,
   };
   for (cat::idx bit_index = 0u; bit_index < 10u; ++bit_index) {
      cross_word_bits[bit_index] = cross_word_values[bit_index];
   }
   cross_word_bits.reverse_inplace();
   for (cat::idx bit_index = 0u; bit_index < 10u; ++bit_index) {
      cat::verify(
         cross_word_bits[bit_index] == cross_word_values[9u - bit_index]
      );
   }

   right.reverse_inplace();
   cat::verify(right_words[0u] == 0b0010'1100_u1);

   right.rotate_left(2u);
   cat::verify(right_words[0u] == 0b0000'1011_u1);
   right.rotate_right(2u);
   cat::verify(right_words[0u] == 0b0010'1100_u1);

   right.transform([](cat::bit_value bit) -> bool {
      return !bit;
   });
   cat::verify(right_words[0u] == 0b1101'0011_u1);

   right.clear();
   cat::verify(right_words[0u] == 0u);
   right.fill();
   cat::verify(right_words[0u] == cat::uint1::max());
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
