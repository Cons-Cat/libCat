#include <cat/atomic>

#include "../unit_tests.hpp"

namespace {

template <typename Atomic>
concept has_fetch_add = requires(Atomic atomic) { atomic.fetch_add(1); };

template <typename Atomic>
concept has_fetch_and = requires(Atomic atomic) { atomic.fetch_and(1); };

template <typename Atomic>
concept has_increment = requires(Atomic atomic) {
                           ++atomic;
                           atomic++;
                        };

template <typename Atomic>
concept has_runtime_load_order =
   requires(Atomic& atomic) { atomic.load(cat::memory_order::relaxed); };

template <typename Atomic>
concept has_runtime_store_order =
   requires(Atomic& atomic) { atomic.store(1, cat::memory_order::relaxed); };

template <typename T>
concept has_atomic_specialization = requires { sizeof(cat::atomic<T>); };

template <typename Atomic>
concept has_volatile_load =
   requires(Atomic volatile& atomic) { atomic.load(); };

template <typename Atomic>
concept has_volatile_store =
   requires(Atomic volatile& atomic) { atomic.store(1); };

template <typename Atomic>
concept has_volatile_lock_free =
   requires(Atomic volatile& atomic) { atomic.is_lock_free(); };

template <typename Flag>
concept has_volatile_flag_test = requires(Flag volatile& flag) { flag.test(); };

consteval auto
constexpr_atomic_value_operations() -> bool {
   cat::atomic<int> value{3};
   if (value.load() != 3) {
      return false;
   }

   value.store(4, cat::memory_order::relaxed);
   if (value.exchange(5) != 4) {
      return false;
   }

   int expected = 5;
   if (!value.compare_exchange_strong(expected, 6)) {
      return false;
   }

   if (value.fetch_add(2) != 6 || value.fetch_sub(1) != 8) {
      return false;
   }

   if (value.fetch_and(0b1110) != 7 || value.fetch_or(0b0001) != 6
       || value.fetch_xor(0b0011) != 7) {
      return false;
   }

   if (value.fetch_max(9) != 4 || value.fetch_min(5) != 9) {
      return false;
   }

   return value.load() == 5;
}

consteval auto
constexpr_atomic_reference_operations() -> bool {
   int value = 3;
   cat::atomic<int&> reference(value);
   reference.store(4, cat::memory_order::relaxed);
   if (reference.exchange(5) != 4) {
      return false;
   }

   int expected = 5;
   if (!reference.compare_exchange_weak(expected, 6)) {
      return false;
   }

   int bound_value = 1;
   cat::atomic_ref_relaxed<int> bound_reference(bound_value);
   if (bound_reference.exchange(2) != 1 || bound_reference.load() != 2) {
      return false;
   }

   return reference.fetch_add(2) == 6 && value == 8;
}

}  // namespace

$test(atomic_memory_order_helpers) {
   static_assert(cat::memory_order_relaxed == cat::memory_order::relaxed);
   static_assert(cat::memory_order_acquire == cat::memory_order::acquire);
   static_assert(cat::memory_order_release == cat::memory_order::release);
   static_assert(cat::memory_order_acq_rel == cat::memory_order::acq_rel);
   static_assert(cat::memory_order_seq_cst == cat::memory_order::seq_cst);
   static_assert(constexpr_atomic_value_operations());
   static_assert(constexpr_atomic_reference_operations());
   static_assert(cat::detail::cmpexch_failure_order(cat::memory_order::seq_cst)
                 == cat::memory_order::seq_cst);
   static_assert(cat::detail::cmpexch_failure_order(cat::memory_order::acq_rel)
                 == cat::memory_order::acquire);
   static_assert(cat::detail::cmpexch_failure_order(cat::memory_order::release)
                 == cat::memory_order::relaxed);

   cat::thread_fence(cat::memory_order::seq_cst);
   cat::signal_fence(cat::memory_order::seq_cst);
}

$test(atomic_flag) {
   static_assert(cat::atomic_flag::is_always_lock_free);
   static_assert(!cat::is_copy_constructible<cat::atomic_flag>);
   static_assert(!cat::is_copy_assignable<cat::atomic_flag>);
   static_assert(!has_volatile_flag_test<cat::atomic_flag>);

   cat::atomic_flag flag;
   cat::verify(!flag.test(cat::memory_order::relaxed));
   cat::verify(!flag.test_and_set(cat::memory_order::acquire));
   cat::verify(flag.test(cat::memory_order::relaxed));
   cat::verify(flag.test_and_set());
   flag.clear(cat::memory_order::release);
   cat::verify(!flag.test());
   flag.wait(true, cat::memory_order::relaxed);
   flag.notify_one();
   flag.notify_all();
}

$test(atomic_store_load_assignment_conversion) {
   static_assert(cat::atomic<int>::required_alignment >= alignof(int));
   static_assert(cat::atomic<int>::is_always_lock_free);
   static_assert(cat::is_same<cat::atomic<int>::value_type, int>);
   static_assert(cat::is_same<cat::atomic<int>::difference_type, int>);
   static_assert(!has_atomic_specialization<int const>);
   static_assert(!has_atomic_specialization<int volatile>);
   static_assert(!has_atomic_specialization<int const volatile>);
   static_assert(!has_volatile_load<cat::atomic<int>>);
   static_assert(!has_volatile_store<cat::atomic<int>>);
   static_assert(
      cat::is_same<cat::atomic<int*>::difference_type, __PTRDIFF_TYPE__>);

   cat::atomic<int> constructed{3};
   cat::verify(constructed.load() == 3);

   cat::atomic<int> value;
   value.store(4);
   cat::verify(value.load() == 4);
   value.store(5, cat::memory_order::relaxed);
   cat::verify(value.load(cat::memory_order::relaxed) == 5);
   cat::verify((value = 6) == 6);
   int loaded = value;
   cat::verify(loaded == 6);

   cat::atomic<bool> flag{false};
   cat::verify(!flag.load());
   cat::verify((flag = true));
   cat::verify(flag.load());
   static_assert(!has_fetch_add<cat::atomic<bool>>);
   static_assert(!has_fetch_and<cat::atomic<bool>>);
   static_assert(!has_increment<cat::atomic<bool>>);

   cat::atomic<cat::uint4> word;
   word.store(1u);
   cat::verify(word.load() == 1u);
}

$test(atomic_exchange) {
   cat::atomic<int> value;
   value.store(4);
   cat::verify(value.exchange(9) == 4);
   cat::verify(value.load() == 9);
   cat::verify(value.exchange(12, cat::memory_order::relaxed) == 9);
   cat::verify(value.load(cat::memory_order::relaxed) == 12);

   cat::atomic<cat::uint4> word;
   word.store(1u);
   cat::verify(word.exchange(2u) == 1u);
   cat::verify(word.load() == 2u);
}

$test(atomic_compare_exchange) {
   cat::atomic<int> value;
   value.store(10);

   int expected = 9;
   cat::verify(!value.compare_exchange_strong(
      expected, 11, cat::memory_order::seq_cst, cat::memory_order::relaxed));
   cat::verify(expected == 10);
   cat::verify(value.load() == 10);

   expected = 10;
   cat::verify(value.compare_exchange_strong(
      expected, 11, cat::memory_order::seq_cst, cat::memory_order::relaxed));
   cat::verify(expected == 10);
   cat::verify(value.load() == 11);

   expected = 12;
   cat::verify(!value.compare_exchange_strong(expected, 13));
   cat::verify(expected == 11);

   expected = 11;
   bool weak_success = false;
   for (idx attempt = 0u; attempt < 16u && !weak_success; ++attempt) {
      weak_success = value.compare_exchange_weak(
         expected, 14, cat::memory_order::seq_cst, cat::memory_order::relaxed);
   }
   cat::verify(weak_success);
   cat::verify(value.load() == 14);

   expected = 15;
   cat::verify(!value.compare_exchange_weak(expected, 16));
   cat::verify(expected == 14);

   cat::atomic<bool> flag;
   flag.store(false);
   bool expected_flag = true;
   cat::verify(!flag.compare_exchange_strong(expected_flag, true));
   cat::verify(!expected_flag);
   cat::verify(flag.compare_exchange_strong(expected_flag, true));
   cat::verify(flag.load());
}

$test(atomic_arithmetic_operators) {
   cat::atomic<int> value;
   value.store(0);

   cat::verify(value++ == 0);
   cat::verify(value.load() == 1);
   cat::verify(++value == 2);
   cat::verify(value-- == 2);
   cat::verify(value.load() == 1);
   cat::verify(--value == 0);

   cat::verify((value += 5) == 5);
   cat::verify((value -= 2) == 3);
   cat::verify((value |= 0b1000) == 0b1011);
   cat::verify((value &= 0b1010) == 0b1010);
   cat::verify((value ^= 0b0011) == 0b1001);
}

$test(atomic_fetch_operations) {
   cat::atomic<int> value;
   value.store(10);

   cat::verify(value.fetch_add(5) == 10);
   cat::verify(value.load() == 15);
   cat::verify(value.fetch_sub(3, cat::memory_order::relaxed) == 15);
   cat::verify(value.load() == 12);
   cat::verify(value.fetch_and(0b1010) == 12);
   cat::verify(value.load() == 0b1000);
   cat::verify(value.fetch_or(0b0011, cat::memory_order::relaxed) == 0b1000);
   cat::verify(value.load() == 0b1011);
   cat::verify(value.fetch_xor(0b0110) == 0b1011);
   cat::verify(value.load() == 0b1101);
   cat::verify(value.fetch_max(20) == 0b1101);
   cat::verify(value.load() == 20);
   cat::verify(value.fetch_min(15, cat::memory_order::relaxed) == 20);
   cat::verify(value.load() == 15);
   value.wait(14, cat::memory_order::relaxed);
   value.notify_one();
   value.notify_all();

   cat::atomic<cat::uint4> word;
   word.store(1u);
   cat::verify(word.fetch_add(2u) == 1u);
   cat::verify(word.load() == 3u);

   static_assert(!has_fetch_and<cat::atomic<cat::float4>>);

   static_assert(!has_fetch_and<cat::atomic<int*>>);
}

$test(atomic_overflow_semantics) {
   cat::atomic<cat::uint1> byte;
   byte.store(254u);
   cat::verify(byte.sat().fetch_add(2u) == 254u);
   cat::verify(byte.load() == cat::limits<cat::uint1>::max());
   cat::verify(byte.wrap().fetch_add(1u) == cat::limits<cat::uint1>::max());
   cat::verify(byte.load() == 0u);
   cat::verify(byte.sat().fetch_sub(1u) == 0u);
   cat::verify(byte.load() == 0u);

   cat::atomic<cat::sat_uint1> sat_byte;
   sat_byte.store(cat::limits<cat::sat_uint1>::max());
   cat::verify(sat_byte.fetch_add(1u) == cat::limits<cat::sat_uint1>::max());
   cat::verify(sat_byte.load() == cat::limits<cat::sat_uint1>::max());
   cat::verify((sat_byte -= 1u) == 254u);
   cat::verify(sat_byte.wrap().fetch_add(2u) == 254u);
   cat::verify(sat_byte.load() == 0u);

   cat::atomic<cat::int1> signed_byte;
   signed_byte.store(cat::limits<cat::int1>::max());
   cat::verify(signed_byte.sat().fetch_add(1) == cat::limits<cat::int1>::max());
   cat::verify(signed_byte.load() == cat::limits<cat::int1>::max());
   signed_byte.store(cat::limits<cat::int1>::min());
   cat::verify(signed_byte.sat().fetch_sub(1) == cat::limits<cat::int1>::min());
   cat::verify(signed_byte.load() == cat::limits<cat::int1>::min());

   cat::sat_uint1 storage = 254u;
   cat::atomic<cat::sat_uint1&> reference{storage};
   cat::verify(reference.fetch_add(2u) == 254u);
   cat::verify(storage == cat::limits<cat::sat_uint1>::max());
   cat::verify(reference.wrap().fetch_add(1u)
               == cat::limits<cat::sat_uint1>::max());
   cat::verify(storage == 0u);
}

$test(atomic_ref_bound) {
   static_assert(cat::atomic_ref_relaxed<int>::memory_ordering
                 == cat::memory_order::relaxed);
   static_assert(cat::atomic_ref_acq_rel<int>::memory_ordering
                 == cat::memory_order::acq_rel);
   static_assert(cat::atomic_ref_seq_cst<int>::memory_ordering
                 == cat::memory_order::seq_cst);
   static_assert(cat::is_copy_constructible<cat::atomic_ref_relaxed<int>>);
   static_assert(!cat::is_copy_assignable<cat::atomic_ref_relaxed<int>>);
   static_assert(!cat::is_constructible<cat::atomic_ref_relaxed<int>,
                                        cat::atomic_ref_seq_cst<int> const&>);
   static_assert(!has_runtime_load_order<cat::atomic_ref_relaxed<int>>);
   static_assert(!has_runtime_store_order<cat::atomic_ref_relaxed<int>>);

   int storage = 0;
   cat::atomic_ref_relaxed<int> relaxed{storage};
   relaxed.store(1);
   cat::verify(storage == 1);
   cat::verify(relaxed.load() == 1);
   cat::verify(relaxed.exchange(3) == 1);
   cat::verify(storage == 3);

   int expected = 2;
   cat::verify(!relaxed.compare_exchange_strong(expected, 4));
   cat::verify(expected == 3);
   cat::verify(relaxed.compare_exchange_strong(expected, 4));
   cat::verify(storage == 4);

   cat::verify(relaxed.fetch_add(2) == 4);
   cat::verify(++relaxed == 7);
   cat::verify(relaxed-- == 7);
   cat::verify((relaxed += 5) == 11);
   cat::verify((relaxed -= 2) == 9);
   cat::verify((relaxed |= 0b0100) == 13);
   cat::verify((relaxed &= 0b0111) == 5);
   cat::verify((relaxed ^= 0b0011) == 6);
   cat::verify(relaxed.fetch_max(10) == 6);
   cat::verify(storage == 10);
   cat::verify(relaxed.fetch_min(4) == 10);
   cat::verify(storage == 4);
   relaxed.wait(3);
   relaxed.notify_one();
   relaxed.notify_all();

   cat::atomic_ref_acq_rel<int> acq_rel{storage};
   acq_rel.store(8);
   cat::verify(acq_rel.load() == 8);
   cat::verify(acq_rel.fetch_sub(3) == 8);
   cat::verify(storage == 5);

   cat::uint4 word_storage = 1u;
   cat::atomic_ref_seq_cst<cat::uint4> word{word_storage};
   cat::verify(word.fetch_add(2u) == 1u);
   cat::verify(word.load() == 3u);

   cat::sat_uint1 sat_storage = 254u;
   cat::atomic_ref_relaxed<cat::sat_uint1> sat_ref{sat_storage};
   cat::verify(sat_ref.fetch_add(2u) == 254u);
   cat::verify(sat_storage == cat::limits<cat::sat_uint1>::max());
}

$test(atomic_accessor) {
   int values[2] = {0, 0};

   cat::atomic_accessor<int> generic_accessor;
   auto generic_reference = generic_accessor.access(values, 0u);
   generic_reference.store(3, cat::memory_order::relaxed);
   cat::verify(values[0] == 3);
   cat::verify(generic_accessor.offset(values, 1u) == values + 1u);

   cat::atomic_accessor_relaxed<int> relaxed_accessor;
   auto relaxed_reference = relaxed_accessor.access(values, 1u);
   relaxed_reference.store(5);
   cat::verify(values[1] == 5);
   cat::verify(relaxed_accessor.offset(values, 1u) == values + 1u);

   cat::atomic_accessor_acq_rel<int> acq_rel_accessor;
   auto acq_rel_reference = acq_rel_accessor.access(values, 0u);
   cat::verify(acq_rel_reference.exchange(7) == 3);
   cat::verify(values[0] == 7);

   cat::atomic_accessor_seq_cst<int> seq_cst_accessor;
   auto seq_cst_reference = seq_cst_accessor.access(values, 1u);
   cat::verify(seq_cst_reference.fetch_add(2) == 5);
   cat::verify(values[1] == 7);
}

$test(atomic_reference) {
   static_assert(!has_atomic_specialization<int const&>);
   static_assert(!has_atomic_specialization<int volatile&>);
   static_assert(!has_volatile_load<cat::atomic<int&>>);
   static_assert(!has_volatile_store<cat::atomic<int&>>);

   int storage = 1;
   cat::atomic<int&> reference{storage};
   cat::atomic<int&> second_reference{storage};
   cat::atomic<int&> copied_reference = reference;
   static_assert(cat::atomic<int&>::required_alignment >= alignof(int));
   static_assert(cat::atomic<int&>::is_always_lock_free);
   static_assert(cat::is_same<cat::atomic<int&>::value_type, int>);

   cat::verify(reference.load() == 1);
   reference.store(2);
   cat::verify(storage == 2);
   cat::verify(second_reference.exchange(3) == 2);
   cat::verify(copied_reference.exchange(3) == 3);
   cat::verify(storage == 3);
   cat::verify(reference.fetch_add(4) == 3);
   cat::verify(storage == 7);
   cat::verify((reference -= 2) == 5);
   cat::verify(storage == 5);

   int expected = 4;
   cat::verify(!reference.compare_exchange_strong(expected, 9));
   cat::verify(expected == 5);
   cat::verify(storage == 5);
   cat::verify(reference.compare_exchange_strong(expected, 9));
   cat::verify(storage == 9);
   cat::verify(reference.fetch_max(12) == 9);
   cat::verify(storage == 12);
   cat::verify(reference.fetch_min(10, cat::memory_order::relaxed) == 12);
   cat::verify(storage == 10);
   reference.wait(9, cat::memory_order::relaxed);
   reference.notify_one();
   reference.notify_all();

   cat::uint4 word_storage = 1u;
   cat::atomic<cat::uint4&> word_reference{word_storage};
   cat::verify(word_reference.fetch_add(2u) == 1u);
   cat::verify(word_storage == 3u);
   cat::verify(word_reference.exchange(4u) == 3u);
   cat::verify(word_storage == 4u);

   bool flag_storage = false;
   cat::atomic<bool&> flag_reference{flag_storage};
   flag_reference.store(true);
   cat::verify(flag_storage);
   bool expected_flag = false;
   cat::verify(!flag_reference.compare_exchange_strong(expected_flag, false));
   cat::verify(expected_flag);
   cat::verify(flag_reference.compare_exchange_strong(expected_flag, false));
   cat::verify(!flag_storage);
   static_assert(!has_fetch_add<cat::atomic<bool&>>);
   static_assert(!has_increment<cat::atomic<bool&>>);
}

$test(atomic_lock_free) {
   static_assert(!has_volatile_lock_free<cat::atomic<int>>);
   static_assert(!has_volatile_lock_free<cat::atomic<int&>>);

   cat::atomic<int> value;
   [[maybe_unused]]
   bool const lock_free = value.is_lock_free();

   int storage = 0;
   cat::atomic<int&> reference{storage};
   [[maybe_unused]]
   bool const reference_lock_free = reference.is_lock_free();
}
