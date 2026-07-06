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

template <typename T>
concept has_atomic_ref_relaxed_specialization =
   requires { sizeof(cat::atomic_ref_relaxed<T>); };

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

template <typename Atomic>
concept has_fetch_fmaximum =
   requires(Atomic atomic) { atomic.fetch_fmaximum(1.0f); };

template <typename Atomic>
concept has_address = requires(Atomic atomic) { atomic.address(); };

template <typename Atomic>
concept has_store_add = requires(Atomic atomic) { atomic.store_add(1); };

consteval auto
constexpr_atomic_value_operations() -> bool {
   cat::atomic<int> value{3};
   if (value.load() != 3) {
      return false;
   }

   if (
      !cat::atomic<int>::notify_is_lock_free()
      || !cat::atomic<int>::wait_is_signal_safe()
   ) {
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

   if (
      value.fetch_and(0b1110) != 7 || value.fetch_or(0b0001) != 6
      || value.fetch_xor(0b0011) != 7
   ) {
      return false;
   }

   if (value.fetch_max(9) != 4 || value.fetch_min(5) != 9) {
      return false;
   }

   value.store(5);
   if (!value.compare(5) || value.compare(7)) {
      return false;
   }

   int compare_expected = 5;
   if (!value.compare_load(compare_expected) || compare_expected != 5) {
      return false;
   }

   compare_expected = 9;
   if (value.compare_load(compare_expected) || compare_expected != 5) {
      return false;
   }

   value.store(0);
   value.store_add(7);
   if (value.load() != 7) {
      return false;
   }
   value.store_sub(2, cat::memory_order::release);
   value.store_and(0b0111);
   value.store_or(0b1000);
   value.store_xor(0b0011);
   value.store_max(20);
   value.store_min(15);
   if (value.load() != 15) {
      return false;
   }

   return value.load() == 15;
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

   if (!bound_reference.compare(2) || bound_reference.compare(3)) {
      return false;
   }

   int bound_expected = 9;
   if (bound_reference.compare_load(bound_expected) || bound_expected != 2) {
      return false;
   }

   reference.store(6);
   if (!reference.compare(6) || reference.compare(0)) {
      return false;
   }

   int reference_expected = 0;
   if (reference.compare_load(reference_expected) || reference_expected != 6) {
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
   static_assert(
      cat::detail::cmpexch_failure_order(cat::memory_order::seq_cst)
      == cat::memory_order::seq_cst
   );
   static_assert(
      cat::detail::cmpexch_failure_order(cat::memory_order::acq_rel)
      == cat::memory_order::acquire
   );
   static_assert(
      cat::detail::cmpexch_failure_order(cat::memory_order::release)
      == cat::memory_order::relaxed
   );

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
      cat::is_same<cat::atomic<int*>::difference_type, __PTRDIFF_TYPE__>
   );

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
      expected, 11, cat::memory_order::seq_cst, cat::memory_order::relaxed
   ));
   cat::verify(expected == 10);
   cat::verify(value.load() == 10);

   expected = 10;
   cat::verify(value.compare_exchange_strong(
      expected, 11, cat::memory_order::seq_cst, cat::memory_order::relaxed
   ));
   cat::verify(expected == 10);
   cat::verify(value.load() == 11);

   expected = 12;
   cat::verify(!value.compare_exchange_strong(expected, 13));
   cat::verify(expected == 11);

   expected = 11;
   bool weak_success = false;
   for (idx attempt = 0u; attempt < 16u && !weak_success; ++attempt) {
      weak_success = value.compare_exchange_weak(
         expected, 14, cat::memory_order::seq_cst, cat::memory_order::relaxed
      );
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

$test(atomic_notify_and_wait_traits) {
   static_assert(cat::atomic_flag::notify_is_always_lock_free);
   static_assert(cat::atomic_flag::wait_is_always_signal_safe);
   static_assert(cat::atomic_flag::notify_is_lock_free());
   static_assert(cat::atomic_flag::wait_is_signal_safe());

   static_assert(cat::atomic<int>::notify_is_always_lock_free);
   static_assert(cat::atomic<int>::wait_is_always_signal_safe);
   static_assert(cat::atomic<int>::notify_is_lock_free());
   static_assert(cat::atomic<int>::wait_is_signal_safe());

   static_assert(cat::atomic<int&>::notify_is_always_lock_free);
   static_assert(cat::atomic<int&>::wait_is_always_signal_safe);
   static_assert(cat::atomic<int&>::notify_is_lock_free());
   static_assert(cat::atomic<int&>::wait_is_signal_safe());

   static_assert(cat::atomic_ref_relaxed<int>::notify_is_always_lock_free);
   static_assert(cat::atomic_ref_relaxed<int>::wait_is_always_signal_safe);
   static_assert(cat::atomic_ref_relaxed<int>::notify_is_lock_free());
   static_assert(cat::atomic_ref_relaxed<int>::wait_is_signal_safe());
}

$test(atomic_store_reductions) {
   cat::atomic<int> value;
   value.store(10);

   value.store_add(5);
   cat::verify(value.load() == 15);
   value.store_add(3, cat::memory_order::relaxed);
   cat::verify(value.load() == 18);
   value.store_sub(8, cat::memory_order::release);
   cat::verify(value.load() == 10);

   value.store_and(0b1110);
   cat::verify(value.load() == 0b1010);
   value.store_or(0b0001, cat::memory_order::relaxed);
   cat::verify(value.load() == 0b1011);
   value.store_xor(0b0110);
   cat::verify(value.load() == 0b1101);

   value.store_max(20);
   cat::verify(value.load() == 20);
   value.store_min(7, cat::memory_order::relaxed);
   cat::verify(value.load() == 7);

   static_assert(!has_store_add<cat::atomic<bool>>);

   int storage = 1;
   cat::atomic<int&> reference{storage};
   reference.store_add(4);
   cat::verify(storage == 5);
   reference.store_max(20);
   cat::verify(storage == 20);
   reference.store_min(11, cat::memory_order::relaxed);
   cat::verify(storage == 11);

   int bound_storage = 0;
   cat::atomic_ref_relaxed<int> bound{bound_storage};
   bound.store_add(7);
   bound.store_or(0b1000);
   bound.store_and(0b1110);
   bound.store_xor(0b0001);
   bound.store_max(20);
   bound.store_min(11);
   cat::verify(bound_storage == 11);

   cat::atomic<cat::uint4> word;
   word.store(1u);
   word.store_add(2u);
   cat::verify(word.load() == 3u);

   cat::atomic<cat::uint1> saturating;
   saturating.store(254u);
   saturating.sat().store_add(5u);
   cat::verify(saturating.load() == cat::limits<cat::uint1>::max());
   saturating.wrap().store_add(2u);
   cat::verify(saturating.load() == 1u);
}

$test(atomic_address) {
   int storage = 0;
   cat::atomic<int&> reference{storage};
   cat::verify(reference.address() == static_cast<void*>(&storage));
   static_assert(cat::is_same<decltype(reference.address()), void*>);
   static_assert(!has_address<cat::atomic<int>>);

   cat::atomic_ref_relaxed<int> bound{storage};
   cat::verify(bound.address() == static_cast<void*>(&storage));
   static_assert(cat::is_same<decltype(bound.address()), void*>);
}

$test(atomic_floating_point_min_max) {
   cat::atomic<cat::float4> value;
   value.store(2.0f);
   cat::verify(value.fetch_fmaximum(5.0f) == 2.0f);
   cat::verify(value.load() == 5.0f);
   cat::verify(value.fetch_fminimum(1.5f) == 5.0f);
   cat::verify(value.load() == 1.5f);

   value.store(__builtin_nanf(""));
   cat::float4 const propagated = value.fetch_fmaximum(2.0f);
   cat::verify(__builtin_isnan(static_cast<float>(propagated)));
   cat::verify(__builtin_isnan(static_cast<float>(value.load())));

   value.store(__builtin_nanf(""));
   cat::verify(
      __builtin_isnan(static_cast<float>(value.fetch_fmaximum_num(3.0f)))
   );
   cat::verify(value.load() == 3.0f);
   value.store(3.0f);
   cat::verify(value.fetch_fmaximum_num(__builtin_nanf("")) == 3.0f);
   cat::verify(value.load() == 3.0f);

   value.store(-0.0f);
   cat::verify(value.fetch_fmaximum(0.0f) == -0.0f);
   cat::verify(value.load() == 0.0f);
   value.store(0.0f);
   cat::verify(value.fetch_fminimum(-0.0f) == 0.0f);
   cat::verify(__builtin_signbit(static_cast<float>(value.load())));

   float storage = 1.0f;
   cat::atomic<cat::float4&> reference{
      *reinterpret_cast<cat::float4*>(&storage)
   };
   reference.fetch_fmaximum_num(__builtin_nanf(""));
   cat::verify(storage == 1.0f);

   cat::atomic_ref_relaxed<cat::float4> bound{
      *reinterpret_cast<cat::float4*>(&storage)
   };
   bound.fetch_fminimum(0.5f);
   cat::verify(storage == 0.5f);

   static_assert(has_fetch_fmaximum<cat::atomic<cat::float4>>);
   static_assert(!has_fetch_fmaximum<cat::atomic<int>>);
}

$test(atomic_compare_and_compare_load) {
   cat::atomic<int> value;
   value.store(10);

   cat::verify(value.compare(10));
   cat::verify(value.compare(10, cat::memory_order::relaxed));
   cat::verify(
      value.compare(10, cat::memory_order::seq_cst, cat::memory_order::relaxed)
   );
   cat::verify(!value.compare(11));
   cat::verify(value.load() == 10);

   int expected = 10;
   cat::verify(value.compare_load(expected));
   cat::verify(expected == 10);
   cat::verify(value.compare_load(expected, cat::memory_order::acquire));
   cat::verify(expected == 10);

   expected = 11;
   cat::verify(!value.compare_load(expected));
   cat::verify(expected == 10);
   cat::verify(value.load() == 10);

   expected = 17;
   cat::verify(!value.compare_load(
      expected, cat::memory_order::seq_cst, cat::memory_order::relaxed
   ));
   cat::verify(expected == 10);

   cat::atomic<bool> flag;
   flag.store(true);
   cat::verify(flag.compare(true));
   cat::verify(!flag.compare(false));

   bool flag_expected = false;
   cat::verify(!flag.compare_load(flag_expected));
   cat::verify(flag_expected);
   cat::verify(flag.compare_load(flag_expected));

   cat::atomic<cat::uint4> word;
   word.store(7u);
   cat::verify(word.compare(7u));
   cat::uint4 word_expected = 0u;
   cat::verify(!word.compare_load(word_expected));
   cat::verify(word_expected == 7u);

   int storage = 4;
   cat::atomic<int&> reference{storage};
   cat::verify(reference.compare(4));
   cat::verify(!reference.compare(5, cat::memory_order::relaxed));

   int reference_expected = 1;
   cat::verify(!reference.compare_load(reference_expected));
   cat::verify(reference_expected == 4);
   cat::verify(reference.compare_load(reference_expected));
   cat::verify(storage == 4);

   int bound_storage = 9;
   cat::atomic_ref_relaxed<int> bound{bound_storage};
   cat::verify(bound.compare(9));
   cat::verify(!bound.compare(0));

   int bound_expected = 0;
   cat::verify(!bound.compare_load(bound_expected));
   cat::verify(bound_expected == 9);
   cat::verify(bound.compare_load(bound_expected));
   cat::verify(bound_storage == 9);

   cat::atomic_ref_seq_cst<int> seq_cst_bound{bound_storage};
   cat::verify(seq_cst_bound.compare(9));
   int seq_cst_expected = 1;
   cat::verify(!seq_cst_bound.compare_load(seq_cst_expected));
   cat::verify(seq_cst_expected == 9);
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
   cat::verify(
      reference.wrap().fetch_add(1u) == cat::limits<cat::sat_uint1>::max()
   );
   cat::verify(storage == 0u);
}

$test(atomic_ref_bound) {
   static_assert(
      cat::atomic_ref_relaxed<int>::memory_ordering
      == cat::memory_order::relaxed
   );
   static_assert(
      cat::atomic_ref_acq_rel<int>::memory_ordering
      == cat::memory_order::acq_rel
   );
   static_assert(
      cat::atomic_ref_seq_cst<int>::memory_ordering
      == cat::memory_order::seq_cst
   );
   static_assert(cat::is_copy_constructible<cat::atomic_ref_relaxed<int>>);
   static_assert(!cat::is_copy_assignable<cat::atomic_ref_relaxed<int>>);
   static_assert(
      !cat::is_constructible<
         cat::atomic_ref_relaxed<int>, cat::atomic_ref_seq_cst<int> const&>
   );
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
   // P3323R1 allows const-qualified `T` in `atomic<T&>`.
   static_assert(has_atomic_specialization<int const&>);
   static_assert(!has_atomic_specialization<int volatile&>);
   static_assert(!has_atomic_specialization<int const volatile&>);
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

// P3323R1 / P3860R1 testing surface. These concepts probe operations that
// must be present for `atomic<T&>` but absent for `atomic<T const&>`.
template <typename Atomic>
concept has_store = requires(Atomic atomic) { atomic.store(0); };

template <typename Atomic>
concept has_exchange = requires(Atomic atomic) { atomic.exchange(0); };

template <typename Atomic>
concept has_compare_exchange_strong =
   requires(Atomic atomic, int& x) { atomic.compare_exchange_strong(x, 0); };

template <typename Atomic>
concept has_notify_one = requires(Atomic atomic) { atomic.notify_one(); };

template <typename Atomic>
concept has_assignment = requires(Atomic atomic) { atomic.operator=(0); };

template <typename From, typename To>
concept is_atomic_ref_convertible_from = requires(From from) { To{from}; };

$test(atomic_reference_const_qualified) {
   using cat::atomic;

   // P3323R1 exposes `value_type` stripped of const.
   static_assert(cat::is_same<atomic<int const&>::value_type, int>);

   // `address()` propagates const to the returned `void*` form.
   static_assert(
      cat::is_same<decltype(cat::declval<atomic<int&>>().address()), void*>
   );
   static_assert(
      cat::is_same<
         decltype(cat::declval<atomic<int const&>>().address()), void const*>
   );

   // Mutating ops are gone for `T const`, but loads, waits, compares, and
   // const-only introspection remain.
   static_assert(!has_store<atomic<int const&>>);
   static_assert(!has_exchange<atomic<int const&>>);
   static_assert(!has_compare_exchange_strong<atomic<int const&>>);
   static_assert(!has_fetch_add<atomic<int const&>>);
   static_assert(!has_fetch_and<atomic<int const&>>);
   static_assert(!has_increment<atomic<int const&>>);
   static_assert(!has_store_add<atomic<int const&>>);
   static_assert(!has_notify_one<atomic<int const&>>);
   static_assert(!has_assignment<atomic<int const&>>);

   // Read-only ops on a const reference observe the underlying value.
   int storage = 42;
   atomic<int const&> read_only{storage};
   cat::verify(read_only.load() == 42);
   cat::verify(read_only.compare(42));
   int probe = 0;
   cat::verify(!read_only.compare_load(probe));
   cat::verify(probe == 42);
   cat::verify(read_only.address() == &storage);
   read_only.wait(0, cat::memory_order::relaxed);

   // P3860R1: `atomic<int const&>` is constructible from `atomic<int&>`,
   // but the reverse is not allowed.
   atomic<int&> writable{storage};
   atomic<int const&> from_writable{writable};
   cat::verify(from_writable.load() == 42);
   cat::verify(from_writable.address() == writable.address());

   static_assert(
      is_atomic_ref_convertible_from<atomic<int&>, atomic<int const&>>
   );
   static_assert(
      !is_atomic_ref_convertible_from<atomic<int const&>, atomic<int&>>
   );
   static_assert(!has_atomic_ref_relaxed_specialization<int volatile>);
   static_assert(!has_atomic_ref_relaxed_specialization<int const volatile>);

   // Bound atomic ref carries the same restrictions.
   cat::atomic_ref_relaxed<int const> bound_read_only{storage};
   cat::verify(bound_read_only.load() == 42);
   cat::verify(bound_read_only.compare(42));
   cat::verify(bound_read_only.address() == &storage);
   static_assert(
      cat::is_same<decltype(bound_read_only.address()), void const*>
   );
   static_assert(
      cat::is_same<cat::atomic_ref_relaxed<int const>::value_type, int>
   );
   static_assert(!has_store<cat::atomic_ref_relaxed<int const>>);
   static_assert(!has_fetch_add<cat::atomic_ref_relaxed<int const>>);
   static_assert(!has_store_add<cat::atomic_ref_relaxed<int const>>);
   static_assert(!has_notify_one<cat::atomic_ref_relaxed<int const>>);

   cat::atomic_ref_relaxed<int> writable_bound{storage};
   cat::atomic_ref_relaxed<int const> bound_from_writable{writable_bound};
   cat::verify(bound_from_writable.load() == 42);
   static_assert(
      is_atomic_ref_convertible_from<
         cat::atomic_ref_relaxed<int>, cat::atomic_ref_relaxed<int const>>
   );
   static_assert(
      !is_atomic_ref_convertible_from<
         cat::atomic_ref_relaxed<int const>, cat::atomic_ref_relaxed<int>>
   );
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
