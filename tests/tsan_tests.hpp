#pragma once

#if __has_feature(thread_sanitizer)

#include <cat/atomic>
#include <cat/linux>
#include <cat/thread>

#include "./unit_tests.hpp"

enum class [[clang::enum_extensibility(
   closed
)]] tsan_report : cat::uint1::raw_type {
   none,
   data_race,
   lock_order_inversion,
};

inline constexpr idx tsan_stack_size = 16_uki;

namespace detail {
inline constinit cat::atomic<bool> is_expecting_tsan_reports;
inline constinit cat::atomic<idx> tsan_data_races;
inline constinit cat::atomic<idx> tsan_lock_order_inversions;
inline constinit cat::atomic<idx> tsan_other_reports;
}  // namespace detail

// Run `callback` on a fresh thread and verify it reported only `expected`.
void
expect_tsan_report(tsan_report expected, auto&& callback) {
   detail::tsan_data_races.store(0u);
   detail::tsan_lock_order_inversions.store(0u);
   detail::tsan_other_reports.store(0u);
   detail::is_expecting_tsan_reports.store(true);

   cat::thread runner;
   runner.spawn(g_tests_alloc, 64_uki, callback).verify();
   runner.join().verify();
   runner.free(g_tests_alloc);
   detail::is_expecting_tsan_reports.store(false);

   idx const data_races = detail::tsan_data_races.load();
   idx const lock_order_inversions = detail::tsan_lock_order_inversions.load();
   cat::verify(detail::tsan_other_reports.load() == 0u);
   switch (expected) {
      case tsan_report::none:
         cat::verify(data_races == 0u && lock_order_inversions == 0u);
         break;
      case tsan_report::data_race:
         cat::verify(data_races > 0u && lock_order_inversions == 0u);
         break;
      case tsan_report::lock_order_inversion:
         cat::verify(data_races == 0u && lock_order_inversions > 0u);
         break;
   }
}

// Block until `count` threads arrive, without synchronizing them. Use once.
// Racing threads wait here instead of spinning.
inline void
unsynchronized_barrier(nix::futex& barrier, cat::uint4 count) {
   cat::uint4 arrived =
      barrier.m_value.fetch_add(1u, cat::memory_order::relaxed) + 1u;
   if (arrived == count) {
      auto _ = barrier.wake_all();
      return;
   }

   while (arrived < count) {
      auto _ = barrier.wait(arrived);
      arrived = barrier.m_value.load(cat::memory_order::relaxed);
   }
}

#endif
