#include <cat/bitvec>
#include <cat/iterable>
#include <cat/page_allocator>
#include <cat/raii_bitvec>

#include "../unit_tests.hpp"

$test(bitvec_manual_core) {
   static_assert(cat::is_same<cat::bitvec::storage_type, cat::uword>);
   static_assert(cat::is_same<cat::bitvec::value_type, cat::bit_value>);
   static_assert(
      cat::is_same<cat::bitvec::const_value_type, cat::bit_value const>
   );
   static_assert(
      cat::is_same<cat::bitvec::reference, cat::bit_reference<cat::uword>>
   );
   static_assert(
      cat::is_same<
         cat::bitvec::const_reference, cat::bit_reference<cat::uword const>>
   );
   static_assert(cat::is_same<cat::bitvec::pointer, cat::bit_ptr<cat::uword>>);
   static_assert(
      cat::is_same<cat::bitvec::const_pointer, cat::bit_ptr<cat::uword const>>
   );
   static_assert(cat::is_same<cat::bitvec::size_type, cat::idx>);
   static_assert(cat::is_same<cat::bitvec::difference_type, cat::iword>);
   static_assert(
      cat::is_same<cat::bitvec::iterator, cat::bit_span<cat::uword>::iterator>
   );
   static_assert(
      cat::is_same<
         cat::bitvec::const_iterator, cat::bit_span<cat::uword const>::iterator>
   );
   static_assert(cat::is_same<
                 cat::bitvec::reverse_iterator,
                 cat::bit_span<cat::uword>::reverse_iterator>);
   static_assert(cat::is_same<
                 cat::bitvec::const_reverse_iterator,
                 cat::bit_span<cat::uword const>::reverse_iterator>);
   using bitvec_context = cat::iterable_iteration_context_type<cat::bitvec>;
   static_assert(
      cat::is_same<bitvec_context::element_type, cat::bitvec::reference>
   );

   cat::bitvec bits;
   bits.resize(pager, 10u).verify();
   bits[3u] = true;
   cat::idx manual_range_count = 0u;
   for (cat::bit_reference bit : bits) {
      if (bit) {
         ++manual_range_count;
      }
   }
   cat::verify(bits.size() == 10u);
   cat::verify(bits.popcount() == 1u);
   cat::verify(bits.count() == 10u);
   cat::verify(
      cat::ref(bits)
         .filter([](auto bit) -> bool {
            return bit;
         })
         .count()
      == 1u
   );
   cat::idx reverse_weight = 1u;
   cat::verify(
      (cat::ref(bits)
       | cat::reverse()
       | cat::transform([&reverse_weight](auto bit) -> cat::idx {
            cat::idx const weight = reverse_weight;
            ++reverse_weight;
            return bit ? weight : 0u;
         })
       | cat::sum())
      == 7u
   );
   bits | cat::reverse_inplace();
   cat::verify(bits[6u]);
   bits.reverse_inplace();
   cat::verify(bits[3u]);
   auto bit_slice = cat::slice(bits, 0u, 5u);
   cat::verify(
      bit_slice
         .filter([](auto bit) -> bool {
            return bit;
         })
         .count()
      == 1u
   );
   cat::read_at(bit_slice, 4u).assign(true);
   cat::verify(bits[4u]);
   cat::verify(
      bit_slice
         .transform([](auto bit) -> cat::idx {
            return bit ? 1u : 0u;
         })
         .sum()
      == 2u
   );
   cat::verify(manual_range_count == 1u);
   bits.free(pager);
}

$test(bitvec_core_operations) {
   static_assert(cat::is_same<cat::raii::bitvec::storage_type, cat::uword>);
   static_assert(cat::is_same<cat::raii::bitvec::value_type, cat::bit_value>);
   static_assert(
      cat::is_same<cat::raii::bitvec::const_value_type, cat::bit_value const>
   );
   static_assert(
      cat::is_same<cat::raii::bitvec::reference, cat::bit_reference<cat::uword>>
   );
   static_assert(cat::is_same<
                 cat::raii::bitvec::const_reference,
                 cat::bit_reference<cat::uword const>>);
   static_assert(
      cat::is_same<cat::raii::bitvec::pointer, cat::bit_ptr<cat::uword>>
   );
   static_assert(
      cat::is_same<
         cat::raii::bitvec::const_pointer, cat::bit_ptr<cat::uword const>>
   );
   static_assert(cat::is_same<cat::raii::bitvec::size_type, cat::idx>);
   static_assert(cat::is_same<cat::raii::bitvec::difference_type, cat::iword>);
   static_assert(
      cat::is_same<
         cat::raii::bitvec::iterator, cat::bit_span<cat::uword>::iterator>
   );
   static_assert(cat::is_same<
                 cat::raii::bitvec::const_iterator,
                 cat::bit_span<cat::uword const>::iterator>);
   static_assert(cat::is_same<
                 cat::raii::bitvec::reverse_iterator,
                 cat::bit_span<cat::uword>::reverse_iterator>);
   static_assert(cat::is_same<
                 cat::raii::bitvec::const_reverse_iterator,
                 cat::bit_span<cat::uword const>::reverse_iterator>);
   using raii_bitvec_context =
      cat::iterable_iteration_context_type<cat::raii::bitvec>;
   static_assert(
      cat::is_same<
         raii_bitvec_context::element_type, cat::raii::bitvec::reference>
   );

   cat::raii::bitvec bits{cat::dyn_allocator(pager)};

   cat::verify(bits.size() == 0u);
   cat::verify(bits.word_count() == 0u);
   cat::verify(bits.none());

   bits.resize(70u).verify();
   cat::verify(bits.size() == 70u);
   cat::verify(bits.word_count() == 2u);
   cat::verify(bits.popcount() == 0u);

   bits[0u] = true;
   bits[63u] = true;
   bits[69u] = true;
   cat::verify(bits[0u]);
   cat::verify(bits[63u]);
   cat::verify(bits[69u]);
   cat::idx range_count = 0u;
   for (cat::bit_reference bit : bits) {
      if (bit) {
         ++range_count;
      }
   }
   cat::verify(bits.popcount() == 3u);
   cat::verify(range_count == 3u);
   cat::verify(!bits.has_single_bit());
   cat::verify(bits.countl_one() == 1u);
   cat::verify(bits.countl_zero() == 0u);
   cat::verify(bits.countr_one() == 1u);
   cat::verify(bits.countr_zero() == 0u);
   cat::verify(bits.bit_width() == 70u);
   cat::verify(cat::bit_width(bits) == 70u);
   cat::verify(*bits.cbegin() == true);
   cat::verify(*(bits.cend() - 1u) == true);
   cat::verify(*bits.rbegin() == true);
   cat::verify(*(bits.rend() - 1u) == true);
   cat::verify(*bits.crbegin() == true);
   cat::verify(*(bits.crend() - 1u) == true);
   cat::verify(cat::popcount(bits) == 3u);
   cat::verify(bits.any());
   cat::verify(!bits.all());
   static_assert(cat::is_random_access_collection<cat::bitvec>);
   static_assert(cat::is_random_access_collection<cat::raii::bitvec>);
   cat::verify((bits | cat::count()) == 70u);
   cat::verify((bits | cat::popcount()) == 3u);
   cat::verify(bits.count() == 70u);
   cat::verify(
      cat::ref(bits)
         .filter([](auto bit) -> bool {
            return bit;
         })
         .count()
      == 3u
   );
   cat::idx reverse_weight = 1u;
   cat::verify(
      (cat::ref(bits)
       | cat::reverse()
       | cat::transform([&reverse_weight](auto bit) -> cat::idx {
            cat::idx const weight = reverse_weight;
            ++reverse_weight;
            return bit ? weight : 0u;
         })
       | cat::sum())
      == 78u
   );
   bits | cat::reverse_inplace();
   cat::verify(bits[6u]);
   bits.reverse_inplace();
   cat::verify(bits[63u]);
   auto bit_slice = cat::slice(bits, 60u, 70u);
   cat::verify(
      bit_slice
         .filter([](auto bit) -> bool {
            return bit;
         })
         .count()
      == 2u
   );
   cat::verify(
      bit_slice.reverse()
         .transform([](auto bit) -> cat::idx {
            return bit ? 1u : 0u;
         })
         .sum()
      == 2u
   );

   bits.flip(0u);
   cat::verify(!bits[0u]);
   bits.reset(63u);
   cat::verify(!bits[63u]);
   cat::verify(bits.popcount() == 1u);
   cat::verify(bits.has_single_bit());
   cat::verify(cat::has_single_bit(bits));

   bits.fill();
   cat::verify(bits.popcount() == 70u);
   cat::verify(bits.all());
   cat::verify((bits.data()[1u] & ~((cat::uword(1u) << 6u) - 1u)) == 0u);

   bits.resize(65u).verify();
   cat::verify(bits.size() == 65u);
   cat::verify(bits.popcount() == 65u);
   cat::verify((bits.data()[1u] & ~cat::uword(1u)) == 0u);
}

$test(bitvec_push_pop) {
   cat::raii::bitvec bits{cat::dyn_allocator(pager)};

   bits.reserve(130u).verify();
   cat::verify(bits.capacity() >= 130u);

   for (cat::idx bit_index = 0u; bit_index < 130u; ++bit_index) {
      bits.push_back(bit_index % 3u == 0u).verify();
   }

   cat::verify(bits.size() == 130u);
   cat::verify(bits[0u]);
   cat::verify(!bits[1u]);
   cat::verify(bits[3u]);
   cat::verify(bits.popcount() == 44u);

   cat::verify(bits.pop_back().verify() == true);
   cat::verify(bits.size() == 129u);
   cat::verify(bits.popcount() == 43u);
   cat::verify((bits.data()[2u] & ~cat::uword(1u)) == 0u);
}

$test(bitvec_append_range) {
   cat::array<bool, 5u> source{true, false, true, true, false};

   cat::bitvec manual_bits;
   manual_bits.append_range(pager, source).verify();
   cat::verify(manual_bits.size() == 5u);
   cat::verify(manual_bits.popcount() == 3u);
   cat::verify(manual_bits[0u]);
   cat::verify(!manual_bits[1u]);
   cat::verify(manual_bits[3u]);

   cat::array<cat::uint1, 1u> packed_source{0b0000'0101_u1};
   cat::bit_span packed_bits(packed_source, 0u, 4u);
   manual_bits.append_range(pager, packed_bits).verify();
   cat::verify(manual_bits.size() == 9u);
   cat::verify(manual_bits.popcount() == 5u);
   cat::verify(manual_bits[5u]);
   cat::verify(!manual_bits[6u]);
   manual_bits.free(pager);

   cat::raii::bitvec raii_bits{cat::dyn_allocator(pager)};
   raii_bits.append_range(source).verify();
   cat::verify(raii_bits.size() == 5u);
   cat::verify(raii_bits.popcount() == 3u);
   raii_bits.append_range(packed_bits).verify();
   cat::verify(raii_bits.size() == 9u);
   cat::verify(raii_bits.popcount() == 5u);
}

$test(bitvec_bitwise_predicates) {
   cat::raii::bitvec left{cat::dyn_allocator(pager)};
   cat::raii::bitvec right{cat::dyn_allocator(pager)};
   left.resize(130u).verify();
   right.resize(130u).verify();

   left[0u] = true;
   left[64u] = true;
   right[64u] = true;
   right[129u] = true;

   cat::verify(left.intersects(right));
   cat::verify(!left.disjoint(right));
   cat::verify(!left.is_subset_of(right));
   left[0u] = false;
   cat::verify(left.is_subset_of(right));

   cat::raii::bitvec and_bits = left & right;
   cat::verify(and_bits.popcount() == 1u);
   cat::verify(and_bits[64u]);

   cat::raii::bitvec or_bits = left | right;
   cat::verify(or_bits.popcount() == 2u);
   cat::verify(or_bits[64u]);
   cat::verify(or_bits[129u]);

   cat::raii::bitvec xor_bits = left ^ right;
   cat::verify(xor_bits.popcount() == 1u);
   cat::verify(xor_bits[129u]);

   cat::raii::bitvec not_bits = ~left;
   cat::verify(not_bits.popcount() == 129u);
   cat::verify((not_bits.data()[2u] & ~cat::uword(0b11u)) == 0u);
}
