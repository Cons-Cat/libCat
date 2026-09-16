#include <cat/bitset_shared>
#include <cat/page_allocator>
#include <cat/thread>

#include "../unit_tests.hpp"

namespace {

constexpr cat::idx worker_stack_size = 16_uki;

template <cat::idx bits_count>
void
verify_wait_transitions(cat::idx index) {
   cat::bitset_shared<bits_count> bits;
   cat::atomic<bool> started;
   cat::atomic<cat::idx> phase;
   cat::atomic<bool> completed;
   cat::thread waiter;

   $defer {
      waiter.free(pager);
   };

   waiter
      .spawn(
         pager, worker_stack_size,
         [&bits, &started, &phase, &completed, index] {
            started.release() = true;
            bits.wait(index, false, cat::memory_order::acquire);
            phase.release() = 1u;
            bits[index].wait(true, cat::memory_order::acquire);
            completed.release() = true;
         }
      )
      .verify();

   started.acquire().wait(false);
   bits.set(index, cat::memory_order::release);
   bits.notify_one(index);
   phase.acquire().wait(0u);
   bits.reset(index, cat::memory_order::release);
   bits[index].notify_one();

   waiter.join().verify();
   cat::verify(completed.acquire());
   cat::verify(!bits.test(index, cat::memory_order::relaxed));
}

template <cat::idx bits_count>
void
verify_bitset_shared_value_operations() {
   cat::bitset<bits_count> left_value;
   cat::bitset<bits_count> right_value;
   for (cat::idx index; index < bits_count; ++index) {
      left_value[index] = index % 3u == 0u;
      right_value[index] = index % 5u == 0u;
   }

   cat::bitset_shared<bits_count> left(left_value);
   cat::bitset_shared<bits_count> right(right_value);
   cat::verify(left.load() == left_value);
   cat::verify(static_cast<cat::bitset<bits_count>>(right) == right_value);
   cat::verify(left.equals(left, cat::memory_order::relaxed));
   cat::bitset_shared<bits_count> equal_left(left);
   cat::verify(left == equal_left);
   cat::verify((left != right) == (left_value != right_value));

   cat::verify(
      left.bit_and(right, cat::memory_order::acquire).load()
      == (left_value & right_value)
   );
   cat::verify(
      left.bit_or(right, cat::memory_order::relaxed).load()
      == (left_value | right_value)
   );
   cat::verify((left & right).load() == (left_value & right_value));
   cat::verify((left | right).load() == (left_value | right_value));
   cat::verify((left ^ right).load() == (left_value ^ right_value));
   cat::bitset_shared<bits_count> complement = ~left;
   cat::bitset_shared<bits_count> expected_complement(~left_value);
   cat::verify(complement.load() == ~left_value);
   cat::verify(
      complement.load(complement.storage_array_size - 1u)
      == expected_complement.load(expected_complement.storage_array_size - 1u)
   );

   cat::bitset_shared<bits_count> compound(left);
   compound &= right;
   cat::verify(compound.load() == (left_value & right_value));
   compound = left;
   compound |= right;
   cat::verify(compound.load() == (left_value | right_value));
   compound = left;
   compound ^= right;
   cat::verify(compound.load() == (left_value ^ right_value));

   cat::verify(left.popcount() == left_value.popcount());
   cat::verify(left.has_single_bit() == left_value.has_single_bit());
   cat::verify(left.bit_width() == left_value.bit_width());
   cat::verify(left.countl_zero() == left_value.countl_zero());
   cat::verify(left.countl_one() == left_value.countl_one());
   cat::verify(left.countr_zero() == left_value.countr_zero());
   cat::verify(left.countr_one() == left_value.countr_one());

   cat::verify(
      left.rotate_left(1, cat::memory_order::relaxed).load()
      == left_value.rotate_left(1)
   );
   cat::verify(
      left.rotate_right(-1, cat::memory_order::acquire).load()
      == left_value.rotate_right(-1)
   );

   cat::bitset_shared<bits_count> reversed(left);
   cat::bitset<bits_count> reversed_value = left_value;
   reversed.reverse_inplace();
   reversed_value.reverse_inplace();
   cat::verify(reversed.load() == reversed_value);

   cat::bitset_shared<bits_count> patterns;
   cat::bitset<bits_count> pattern_value;
   patterns.set_even(cat::memory_order::relaxed);
   pattern_value.set_even();
   cat::verify(patterns.load() == pattern_value);
   patterns.set_odd(cat::memory_order::release);
   pattern_value.set_odd();
   cat::verify(patterns.load() == pattern_value);
   patterns.unset_even(cat::memory_order::acq_rel);
   pattern_value.unset_even();
   cat::verify(patterns.load() == pattern_value);
   patterns.unset_odd();
   pattern_value.unset_odd();
   cat::verify(patterns.load() == pattern_value);

   cat::bitset_shared<bits_count> stored;
   stored.store(left_value, cat::memory_order::release);
   cat::verify(stored.load(cat::memory_order::acquire) == left_value);
   stored = right_value;
   cat::verify(stored.load() == right_value);

   cat::bitset_shared<bits_count> copied = left;
   cat::bitset_shared<bits_count> moved = cat::move(copied);
   cat::bitset_shared<bits_count> move_assigned;
   move_assigned = cat::move(moved);
   cat::verify(move_assigned.load() == left_value);

   if constexpr (bits_count != 0u) {
      cat::verify(left.at(0u).has_value());
      cat::verify(
         left.at(0u).value().test(cat::memory_order::relaxed)
         == bool(left_value[0u])
      );
      cat::bitset_shared<bits_count> const& const_left = left;
      cat::verify(const_left.at(0u).value() == bool(left_value[0u]));
   }
   cat::verify(left.at(bits_count).is_empty());
   cat::bitset_shared<bits_count> const& const_left = left;
   cat::verify(const_left.at(bits_count).is_empty());
}

}  // namespace

$test(bitset_shared_storage_layout) {
   using bits7 = cat::bitset_shared<7u>;
   using bits9 = cat::bitset_shared<9u>;
   using bits17 = cat::bitset_shared<17u>;
   using bits65 = cat::bitset_shared<65u>;
   using bits129 = cat::bitset_shared<129u>;

   static_assert(cat::is_same<bits7::value_type, cat::bit_value>);
   static_assert(cat::is_same<bits7::const_value_type, cat::bit_value const>);
   static_assert(cat::is_same<bits7::const_reference, bool>);
   static_assert(cat::is_same<bits7::size_type, cat::idx>);
   static_assert(cat::is_same<bits7::difference_type, cat::iword>);
   static_assert(cat::is_same<bits7::storage_type, cat::uint1>);
   static_assert(cat::is_same<bits9::storage_type, cat::uint2>);
   static_assert(cat::is_same<bits17::storage_type, cat::uint4>);
   static_assert(cat::is_same<bits65::storage_type, cat::uint8>);
   static_assert(bits7::is_always_lock_free);
   static_assert(bits9::is_always_lock_free);
   static_assert(bits17::is_always_lock_free);
   static_assert(bits65::is_always_lock_free);
   static_assert(bits7::storage_array_size == 1u);
   static_assert(bits17::storage_array_size == 1u);
   static_assert(bits65::storage_array_size == 2u);
   static_assert(bits129::storage_array_size == 3u);
   static_assert(bits7::storage_element_size == 1u);
   static_assert(bits17::storage_element_size == 4u);
   static_assert(bits65::storage_element_size == 8u);
   static_assert(bits7::leading_skipped_bits == 1u);
   static_assert(bits65::leading_skipped_bits == 63u);
   static_assert(sizeof(bits7) == 1u);
   static_assert(sizeof(bits17) == 4u);
   static_assert(sizeof(bits65) == 16u);
   static_assert(sizeof(bits129) == 24u);
   static_assert(bits129::size() == 129u);
   static_assert(bits129::notify_is_always_lock_free);
   static_assert(bits129::wait_is_always_signal_safe);
   static_assert(bits129::notify_is_lock_free());
   static_assert(bits129::wait_is_signal_safe());
}

$test(bitset_shared_index_mapping_and_reference) {
   using namespace cat::arithmetic_literals;

   cat::bitset_shared<65u> bits;
   bits.store(1u, cat::uint8(1u) << 63u);
   cat::verify(bits.test(0u, cat::memory_order::relaxed));
   cat::verify(!bits.test(1u));
   cat::verify(!bits.test(64u));

   bits.store(0u, 0x80000000'00000001_u8);
   cat::verify(bits[0u]);
   cat::verify(bits[1u]);
   cat::verify(bits[64u]);

   cat::verify(bits[1u].reset(cat::memory_order::acq_rel));
   cat::verify(!bits[1u].reset());
   cat::verify(!bits[1u].set(cat::memory_order::release));
   cat::verify(bits[1u].set());
   cat::verify(bits[1u].assign(false, cat::memory_order::relaxed));
   cat::verify(!bits[1u].assign(true));
   cat::verify(bits[1u].flip(cat::memory_order::acq_rel));
   cat::verify(!bits[1u].flip());

   bits[2u] = true;
   bits[3u] = bits[2u];
   cat::verify(bits[2u]);
   cat::verify(bits[3u]);
}

$test(bitset_shared_padding_fill_and_queries) {
   using namespace cat::arithmetic_literals;

   cat::bitset_shared<65u> bits;
   bits.store(1u, cat::uint8_max);
   cat::verify(bits.load(1u) == 0x80000000'00000000_u8);
   cat::verify(bits.popcount() == 1u);

   cat::uint8 const prior = bits.fetch_or(1u, cat::uint8_max);
   cat::verify(prior == 0x80000000'00000000_u8);
   cat::verify(bits.load(1u) == prior);

   cat::verify(bits.fetch_and(1u, cat::uint8_max) == prior);
   cat::verify(bits.load(1u) == prior);
   cat::verify(bits.fetch_and(1u, 0_u8) == prior);
   cat::verify(bits.load(1u) == 0_u8);

   bits.store(1u, cat::uint8_max);
   cat::verify(bits.exchange(1u, cat::uint8_max) == prior);
   cat::verify(bits.load(1u) == prior);
   cat::verify(bits.fetch_xor(1u, cat::uint8_max) == prior);
   cat::verify(bits.load(1u) == 0_u8);

   cat::uint8 expected = 0_u8;
   cat::verify(bits.compare_exchange_strong(
      1u, expected, cat::uint8_max, cat::memory_order::release,
      cat::memory_order::relaxed
   ));
   cat::verify(bits.load(1u, cat::memory_order::acquire) == prior);

   bits.fill(cat::memory_order::release);
   cat::verify(bits.load(0u, cat::memory_order::acquire) == cat::uint8_max);
   cat::verify(
      bits.load(1u, cat::memory_order::relaxed) == 0x80000000'00000000_u8
   );
   cat::verify(bits.popcount(cat::memory_order::relaxed) == 65u);
   cat::verify(bits.any_of());
   cat::verify(!bits.none_of());
   cat::verify(bits.all_of());

   bits.fill(false, cat::memory_order::release);
   cat::verify(bits.none_of(cat::memory_order::acquire));
   cat::verify(!bits.any_of());
   cat::verify(!bits.all_of());
}

$test(bitset_shared_copy_and_load) {
   cat::bitset_shared<129u> source;
   source.set(0u);
   source.set(1u);
   source.set(64u);
   source.set(65u);
   source.set(128u);

   cat::bitset_shared<129u> copied = source;
   cat::bitset_shared<129u> assigned;
   assigned = source;

   cat::bitset<129u> const loaded = source.load(cat::memory_order::acquire);
   cat::verify(loaded.popcount() == 5u);
   cat::verify(loaded[0u]);
   cat::verify(loaded[1u]);
   cat::verify(loaded[64u]);
   cat::verify(loaded[65u]);
   cat::verify(loaded[128u]);
   cat::verify(copied.load() == loaded);
   cat::verify(assigned.load() == loaded);

   source.clear();
   cat::verify(source.none_of());
   cat::verify(copied.popcount() == 5u);
   cat::verify(assigned.popcount() == 5u);
}

$test(bitset_shared_wait_boundaries_and_old_values) {
   constexpr cat::idx boundaries[] = {0u, 1u, 64u, 65u, 128u};
   cat::bitset_shared<129u> bits;

   for (cat::idx boundary; boundary < 5u; ++boundary) {
      cat::idx const index = boundaries[boundary.raw];
      bits.wait(index, true, cat::memory_order::relaxed);
      bits[index].wait(true, cat::memory_order::acquire);
      bits.set(index, cat::memory_order::release);
      bits.wait(index, false, cat::memory_order::seq_cst);
      bits[index].wait(false, cat::memory_order::relaxed);
   }
}

$test(bitset_shared_wait_selected_bit_changes) {
   verify_wait_transitions<129u>(64u);
}

$test(bitset_shared_wait_rechecks_same_word_changes) {
   cat::bitset_shared<129u> bits;
   cat::atomic<bool> started;
   cat::atomic<bool> completed;
   cat::thread waiter;

   bits.set(1u, cat::memory_order::relaxed);
   $defer {
      waiter.free(pager);
   };

   waiter
      .spawn(
         pager, worker_stack_size,
         [&bits, &started, &completed] {
            started.release() = true;
            bits.wait(64u, false, cat::memory_order::acquire);
            completed.release() = true;
         }
      )
      .verify();

   started.acquire().wait(false);
   bits.set(2u, cat::memory_order::release);
   bits.notify_one(64u);
   for (cat::idx repetition; repetition < 100'000u; ++repetition) {
      cat::relax_cpu();
   }
   bool const returned_early = completed.acquire();

   bits.set(64u, cat::memory_order::release);
   bits.notify_all(64u);
   waiter.join().verify();

   cat::verify(!returned_early);
   cat::verify(completed.acquire());
}

$test(bitset_shared_wait_ignores_different_word_changes) {
   cat::bitset_shared<129u> bits;
   cat::atomic<bool> started;
   cat::atomic<bool> completed;
   cat::thread waiter;

   $defer {
      waiter.free(pager);
   };

   waiter
      .spawn(
         pager, worker_stack_size,
         [&bits, &started, &completed] {
            started.release() = true;
            bits[64u].wait(false, cat::memory_order::seq_cst);
            completed.release() = true;
         }
      )
      .verify();

   started.acquire().wait(false);
   bits.set(65u, cat::memory_order::release);
   bits.notify_all(65u);
   for (cat::idx repetition; repetition < 100'000u; ++repetition) {
      cat::relax_cpu();
   }
   bool const returned_early = completed.acquire();

   bits.set(64u, cat::memory_order::release);
   bits[64u].notify_one();
   waiter.join().verify();

   cat::verify(!returned_early);
   cat::verify(completed.acquire());
}

$test(bitset_shared_wait_immediate_and_notifications) {
   cat::bitset_shared<129u> bits;
   cat::bitset_shared<129u> const& const_bits = bits;

   const_bits.wait(0u, true, cat::memory_order::relaxed);
   bits[64u].wait(true, cat::memory_order::acquire);
   bits.notify_one(0u);
   bits.notify_all(1u);
   bits.notify_one(64u);
   bits.notify_all(65u);
   bits.notify_one(128u);
   bits[0u].notify_one();
   bits[128u].notify_all();
}

$test(bitset_shared_word_atomic_operations) {
   using namespace cat::arithmetic_literals;

   cat::bitset_shared<65u> bits;
   cat::verify(bits.fetch_or(0u, 0b1010_u8) == 0_u8);
   cat::verify(bits.load(0u) == 0b1010_u8);
   cat::verify(bits.fetch_and(0u, 0b1100_u8) == 0b1010_u8);
   cat::verify(bits.load(0u) == 0b1000_u8);
   cat::verify(
      bits.fetch_xor(0u, 0b0011_u8, cat::memory_order::acq_rel) == 0b1000_u8
   );
   cat::verify(bits.load(0u) == 0b1011_u8);

   cat::uint8 expected = 0b1011_u8;
   cat::uint8 unexpected = 0b1001_u8;
   cat::verify(!bits.compare_exchange_weak(
      0u, unexpected, 0b0101_u8, cat::memory_order::acq_rel,
      cat::memory_order::acquire
   ));
   cat::verify(unexpected == expected);

   while (!bits.compare_exchange_weak(
      0u, expected, 0b0101_u8, cat::memory_order::acq_rel,
      cat::memory_order::acquire
   )) {
      cat::verify(expected == 0b1011_u8);
   }
   cat::verify(bits.load(0u) == 0b0101_u8);

   unexpected = 0b0011_u8;
   cat::verify(!bits.compare_exchange_strong(
      0u, unexpected, 0b1111_u8, cat::memory_order::release,
      cat::memory_order::relaxed
   ));
   cat::verify(unexpected == 0b0101_u8);

   expected = 0b0101_u8;
   cat::verify(bits.compare_exchange_strong(
      0u, expected, 0b1111_u8, cat::memory_order::release,
      cat::memory_order::relaxed
   ));
   cat::verify(bits.load(0u) == 0b1111_u8);

   expected = 0b1111_u8;
   while (!bits.compare_exchange_weak(
      0u, expected, 0b0010_u8, cat::memory_order::relaxed
   )) {
      cat::verify(expected == 0b1111_u8);
   }
   cat::verify(bits.load(0u) == 0b0010_u8);
}

$test(bitset_shared_same_word_contention) {
   cat::bitset_shared<64u> bits;
   cat::thread workers[4];

   $defer {
      for (cat::idx worker; worker < 4u; ++worker) {
         workers[worker.raw].free(pager);
      }
   };

   for (cat::idx worker; worker < 4u; ++worker) {
      workers[worker.raw]
         .spawn(
            pager, worker_stack_size,
            [](cat::bitset_shared<64u>* p_shared, cat::idx offset) {
               for (cat::idx index = offset; index < p_shared->size();
                    index += 4u) {
                  p_shared->set(index, cat::memory_order::relaxed);
               }
            },
            &bits, worker
         )
         .verify();
   }

   for (cat::idx worker; worker < 4u; ++worker) {
      workers[worker.raw].join().verify();
   }

   cat::verify(bits.all_of(cat::memory_order::acquire));
   cat::verify(bits.popcount() == 64u);
}

$test(bitset_shared_disjoint_word_updates) {
   cat::bitset_shared<257u> bits;
   cat::thread workers[4];

   bits.set(0u, cat::memory_order::relaxed);

   $defer {
      for (cat::idx worker; worker < 4u; ++worker) {
         workers[worker.raw].free(pager);
      }
   };

   for (cat::idx worker; worker < 4u; ++worker) {
      cat::idx const begin = 1u + worker * 64u;
      workers[worker.raw]
         .spawn(
            pager, worker_stack_size,
            [](cat::bitset_shared<257u>* p_shared, cat::idx first) {
               for (cat::idx index = first; index < first + 64u; ++index) {
                  p_shared->set(index, cat::memory_order::release);
               }
            },
            &bits, begin
         )
         .verify();
   }

   for (cat::idx worker; worker < 4u; ++worker) {
      workers[worker.raw].join().verify();
   }

   cat::verify(bits.all_of(cat::memory_order::acquire));
   cat::verify(bits.popcount(cat::memory_order::relaxed) == 257u);
}

$test(bitset_shared_value_constructors) {
   using namespace cat::arithmetic_literals;

   constexpr cat::bitset_shared empty("");
   constexpr cat::bitset_shared one("1");
   constexpr cat::bitset_shared word_boundary(
      "1000000000000000000000000000000000000000000000000000000000000001"
   );
   constexpr cat::bitset_shared multiword(
      "10000000000000000000000000000000000000000000000000000000000000001"
   );
   static_assert(empty.size() == 0u);
   static_assert(empty.none_of());
   static_assert(empty.all_of());
   static_assert(one.size() == 1u);
   static_assert(one[0u]);
   static_assert(word_boundary.size() == 64u);
   static_assert(word_boundary[0u]);
   static_assert(word_boundary[63u]);
   static_assert(word_boundary.popcount() == 2u);
   static_assert(multiword.size() == 65u);
   static_assert(multiword[0u]);
   static_assert(multiword[64u]);
   static_assert(multiword.popcount() == 2u);

   constexpr auto from_storage_7 = cat::make_bitset_shared<7u>(0b1010'1010_u1);
   constexpr auto from_storage_64 =
      cat::make_bitset_shared<64u>(0x80000000'00000001_u8);
   constexpr auto from_storage_129 = cat::make_bitset_shared<129u>(
      0x80000000'00000000_u8, 0x80000000'00000000_u8, 0x80000000'00000000_u8
   );
   static_assert(from_storage_7[0u]);
   static_assert(from_storage_7[2u]);
   static_assert(from_storage_7[6u]);
   static_assert(from_storage_64[0u]);
   static_assert(from_storage_64[63u]);
   static_assert(from_storage_129[0u]);
   static_assert(from_storage_129[64u]);
   static_assert(from_storage_129[128u]);
   static_assert(from_storage_129.popcount() == 3u);
}

$test(bitset_shared_value_operator_boundaries) {
   verify_bitset_shared_value_operations<0u>();
   verify_bitset_shared_value_operations<1u>();
   verify_bitset_shared_value_operations<7u>();
   verify_bitset_shared_value_operations<8u>();
   verify_bitset_shared_value_operations<9u>();
   verify_bitset_shared_value_operations<63u>();
   verify_bitset_shared_value_operations<64u>();
   verify_bitset_shared_value_operations<65u>();
   verify_bitset_shared_value_operations<127u>();
   verify_bitset_shared_value_operations<128u>();
   verify_bitset_shared_value_operations<129u>();
}

$test(bitset_shared_compound_operator_contention) {
   constexpr cat::idx workers_count = 4u;
   cat::bitset_shared<129u> bits;
   cat::bitset_shared<129u> masks[workers_count.raw];
   cat::thread workers[workers_count.raw];
   constexpr cat::idx indices[workers_count.raw] = {0u, 63u, 64u, 128u};

   for (cat::idx worker; worker < workers_count; ++worker) {
      masks[worker.raw].set(indices[worker.raw]);
   }

   $defer {
      for (cat::idx worker; worker < workers_count; ++worker) {
         workers[worker.raw].free(pager);
      }
   };

   for (cat::idx worker; worker < workers_count; ++worker) {
      workers[worker.raw]
         .spawn(
            pager, worker_stack_size,
            [](cat::bitset_shared<129u>* p_bits,
               cat::bitset_shared<129u> const* p_mask) {
               *p_bits |= *p_mask;
            },
            &bits, &masks[worker.raw]
         )
         .verify();
   }

   for (cat::idx worker; worker < workers_count; ++worker) {
      workers[worker.raw].join().verify();
   }

   cat::verify(bits.popcount(cat::memory_order::acquire) == workers_count);
   for (cat::idx worker; worker < workers_count; ++worker) {
      cat::verify(bits.test(indices[worker.raw]));
   }
}
