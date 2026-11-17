#include <cat/array>
#include <cat/atomic>
#include <cat/chrono>
#include <cat/semaphore>
#include <cat/thread>

#include "../tsan_tests.hpp"
#include "../unit_tests.hpp"

namespace {

constexpr idx worker_stack_size = 16_uki;

static_assert(cat::semaphore_counting<>::max() == cat::uint4_max);
static_assert(cat::semaphore_binary::max() == 1u);
static_assert(cat::semaphore_counting<4u>::max() == 4u);
// The binary specialization folds its waiters into its futex word.
static_assert(sizeof(cat::semaphore_binary) == sizeof(cat::uint4));
static_assert(sizeof(cat::semaphore_counting<4u>) == sizeof(cat::uint4) * 2u);

template <typename Threads>
void
join_threads(Threads& threads) {
   for (cat::thread& thread : threads) {
      thread.join().verify();
   }
}

template <typename Threads>
void
free_threads(Threads& threads) {
   for (cat::thread& thread : threads) {
      thread.free(g_tests_alloc);
   }
}

}  // namespace

$test(semaphore_counting_try_acquire) {
   cat::semaphore_counting<> semaphore(2u);
   cat::verify(semaphore.try_acquire());
   cat::verify(semaphore.try_acquire());
   cat::verify(!semaphore.try_acquire());

   semaphore.release(2u);
   cat::verify(semaphore.try_acquire());
   cat::verify(semaphore.try_acquire());
   cat::verify(!semaphore.try_acquire());

   semaphore.release();
   semaphore.acquire();
   cat::verify(!semaphore.try_acquire());
}

$test(semaphore_binary_try_acquire) {
   cat::semaphore_binary semaphore(1u);
   cat::verify(semaphore.try_acquire());
   cat::verify(!semaphore.try_acquire());

   semaphore.release(0u);
   cat::verify(!semaphore.try_acquire());

   semaphore.release();
   semaphore.acquire();
   cat::verify(!semaphore.try_acquire());
}

// A semaphore with a small maximum admits at most that many threads at once.
$test(semaphore_counting_bounds_concurrency) {
   constexpr idx thread_count = 8;
   constexpr idx iteration_count = 5'000;
   cat::semaphore_counting<4u> semaphore(4u);
   cat::atomic<idx> inside;
   cat::atomic<bool> exceeded;

   cat::array<cat::thread, thread_count> threads;
   $defer {
      free_threads(threads);
   };

   for (cat::thread& thread : threads) {
      thread
         .spawn(
            g_tests_alloc, worker_stack_size,
            [&] {
               for (idx iteration; iteration < iteration_count; ++iteration) {
                  semaphore.acquire();
                  if (inside.fetch_add(1u) >= 4u) {
                     exceeded.store(true);
                  }
                  inside.fetch_sub(1u);
                  semaphore.release();
               }
            }
         )
         .verify();
   }

   join_threads(threads);
   cat::verify(!exceeded.load());
   for (idx index; index < 4; ++index) {
      cat::verify(semaphore.try_acquire());
   }
   cat::verify(!semaphore.try_acquire());
}

$test(semaphore_counting_try_acquire_times_out) {
   cat::semaphore_binary semaphore(0u);
   cat::verify(!semaphore.try_acquire_for(10 * cat::units::millisecond));

   cat::clock_system::time_point const deadline =
      cat::clock_system::now().verify() + 10 * cat::units::millisecond;
   cat::verify(!semaphore.try_acquire_until(deadline));

   semaphore.release();
   cat::verify(semaphore.try_acquire_for(10 * cat::units::millisecond));
}

$test(semaphore_counting_try_acquire_for_waits) {
   cat::semaphore_binary semaphore(0u);
   cat::atomic<bool> entered;
   cat::atomic<bool> acquired;

   cat::thread worker;
   $defer {
      worker.free(g_tests_alloc);
   };

   worker
      .spawn(
         g_tests_alloc, worker_stack_size,
         [&] {
            entered.release() = true;
            cat::verify(semaphore.try_acquire_for(1 * cat::units::second));
            acquired.release() = true;
         }
      )
      .verify();

   while (!entered.acquire()) {
      cat::machine_pause();
   }

   for (idx iteration; iteration < 1'000; ++iteration) {
      cat::this_thread::yield();
   }

   cat::verify(!acquired.acquire());
   semaphore.release();
   worker.join().verify();
   cat::verify(acquired.acquire());
}

// A single `release(n)` must wake `n` sleeping threads, not only one.
$test(semaphore_counting_release_wakes_many) {
   constexpr cat::uint4 thread_count = 8u;
   cat::semaphore_counting<> semaphore(0u);
   cat::atomic<idx> acquired;

   cat::array<cat::thread, thread_count> threads;
   $defer {
      free_threads(threads);
   };

   for (cat::thread& thread : threads) {
      thread
         .spawn(
            g_tests_alloc, worker_stack_size,
            [&] {
               semaphore.acquire();
               acquired.fetch_add(1u);
            }
         )
         .verify();
   }

   // Give the workers time to fall asleep in the kernel.
   for (idx iteration; iteration < 10'000; ++iteration) {
      cat::this_thread::yield();
   }

   cat::verify(acquired.load() == 0u);
   semaphore.release(thread_count);
   join_threads(threads);
   cat::verify(acquired.load() == thread_count);
   cat::verify(!semaphore.try_acquire());
}

// A binary semaphore excludes like a mutex.
$test(semaphore_binary_counter) {
   constexpr idx thread_count = 8;
   constexpr idx iteration_count = 10'000;
   cat::semaphore_binary semaphore(1u);
   idx counter = 0;

   cat::array<cat::thread, thread_count> threads;
   $defer {
      free_threads(threads);
   };

   for (cat::thread& thread : threads) {
      thread
         .spawn(
            g_tests_alloc, worker_stack_size,
            [&] {
               for (idx iteration; iteration < iteration_count; ++iteration) {
                  semaphore.acquire();
                  ++counter;
                  semaphore.release();
               }
            }
         )
         .verify();
   }

   join_threads(threads);
   cat::verify(counter == thread_count * iteration_count);
}

// Permits pass between producers and consumers without being lost or forged.
$test(semaphore_counting_producer_consumer) {
   constexpr idx pair_count = 4;
   constexpr idx item_count = 10'000;
   cat::semaphore_counting<> semaphore(0u);
   cat::atomic<idx> consumed;

   cat::array<cat::thread, pair_count * 2> threads;
   $defer {
      free_threads(threads);
   };

   for (idx index; index < pair_count; ++index) {
      threads[index * 2]
         .spawn(
            g_tests_alloc, worker_stack_size,
            [&] {
               for (idx item; item < item_count; ++item) {
                  semaphore.release();
               }
            }
         )
         .verify();
      threads[index * 2 + 1]
         .spawn(
            g_tests_alloc, worker_stack_size,
            [&] {
               for (idx item; item < item_count; ++item) {
                  semaphore.acquire();
                  consumed.fetch_add(1u, cat::memory_order::relaxed);
               }
            }
         )
         .verify();
   }

   join_threads(threads);
   cat::verify(consumed.load() == pair_count * item_count);
   cat::verify(!semaphore.try_acquire());
}

// These verify that ThreadSanitizer reports races, so they only run with it.
#if __has_feature(thread_sanitizer)

namespace {

// Two threads hold permits at once and both write `data`. Neither acquire
// follows a release, so nothing orders the writes.
template <typename Semaphore>
void
concurrent_permits(cat::uint4 permits) {
   // Atomics next to `data` would evict its TSan shadow and hide the race.
   alignas(64) Semaphore semaphore(permits);
   alignas(64) int4 data = 0;
   alignas(64) nix::futex acquired{};
   alignas(64) nix::futex written{};
   alignas(64) nix::futex finished{};

   cat::thread worker;
   $defer {
      worker.free(g_tests_alloc);
   };

   worker
      .spawn(
         g_tests_alloc, tsan_stack_size,
         [&] {
            semaphore.acquire();
            unsynchronized_barrier(acquired, 2u);
            unsynchronized_barrier(written, 2u);
            data = 2;
            unsynchronized_barrier(finished, 2u);
            semaphore.release();
         }
      )
      .verify();

   semaphore.acquire();
   unsynchronized_barrier(acquired, 2u);
   data = 1;
   unsynchronized_barrier(written, 2u);
   unsynchronized_barrier(finished, 2u);
   semaphore.release();
   worker.join().verify();
}

// Each thread writes `data` only while holding the semaphore's one permit.
template <typename Semaphore>
void
exclusive_permit() {
   Semaphore semaphore(1u);
   alignas(64) int4 data = 0;
   nix::futex started{};

   cat::thread worker;
   $defer {
      worker.free(g_tests_alloc);
   };

   worker
      .spawn(
         g_tests_alloc, tsan_stack_size,
         [&] {
            unsynchronized_barrier(started, 2u);
            for (idx iteration; iteration < 100; ++iteration) {
               semaphore.acquire();
               ++data;
               semaphore.release();
            }
         }
      )
      .verify();

   unsynchronized_barrier(started, 2u);
   for (idx iteration; iteration < 100; ++iteration) {
      semaphore.acquire();
      ++data;
      semaphore.release();
   }
   worker.join().verify();
}

// A producer writes `data` and releases a permit to a consumer.
template <typename Semaphore>
void
release_publishes() {
   Semaphore semaphore(0u);
   alignas(64) int4 data = 0;

   cat::thread consumer;
   $defer {
      consumer.free(g_tests_alloc);
   };

   consumer
      .spawn(
         g_tests_alloc, tsan_stack_size,
         [&] {
            semaphore.acquire();
            cat::verify(data == 1);
         }
      )
      .verify();

   data = 1;
   semaphore.release();
   consumer.join().verify();
}

// A failed acquire must not synchronize with the thread that holds the permit.
// `attempt` tries to acquire, and must fail. The holder releases after writing
// `data` and then takes its permit back, so a failed acquire that wrongly
// synchronized with that release would hide the race.
template <typename Semaphore>
void
failed_acquire(auto&& attempt) {
   // Atomics next to `data` would evict its TSan shadow and hide the race.
   alignas(64) Semaphore semaphore(1u);
   alignas(64) int4 data = 0;
   alignas(64) nix::futex written{};
   alignas(64) nix::futex attempted{};

   cat::thread holder;
   $defer {
      holder.free(g_tests_alloc);
   };

   holder
      .spawn(
         g_tests_alloc, tsan_stack_size,
         [&] {
            semaphore.acquire();
            data = 1;
            semaphore.release();
            semaphore.acquire();
            unsynchronized_barrier(written, 2u);
            unsynchronized_barrier(attempted, 2u);
            semaphore.release();
         }
      )
      .verify();

   unsynchronized_barrier(written, 2u);
   cat::verify(!attempt(semaphore));
   cat::verify(data == 1);
   unsynchronized_barrier(attempted, 2u);
   holder.join().verify();
}

}  // namespace

$test(tsan_semaphore_counting_concurrent_permits) {
   expect_tsan_report(tsan_report::data_race, [&] {
      concurrent_permits<cat::semaphore_counting<>>(2u);
   });
}

$test(tsan_semaphore_counting_exclusive_permit) {
   expect_tsan_report(tsan_report::none, [&] {
      exclusive_permit<cat::semaphore_counting<4u>>();
   });
}

$test(tsan_semaphore_counting_release_publishes) {
   expect_tsan_report(tsan_report::none, [&] {
      release_publishes<cat::semaphore_counting<>>();
   });
}

$test(tsan_semaphore_counting_failed_try_acquire) {
   expect_tsan_report(tsan_report::data_race, [&] {
      failed_acquire<cat::semaphore_counting<>>([](auto& semaphore) {
         return semaphore.try_acquire();
      });
   });
}

$test(tsan_semaphore_counting_timed_out_acquire) {
   expect_tsan_report(tsan_report::data_race, [&] {
      failed_acquire<cat::semaphore_counting<>>([](auto& semaphore) {
         return semaphore.try_acquire_for(1 * cat::units::millisecond);
      });
   });
}

$test(tsan_semaphore_binary_exclusive_permit) {
   expect_tsan_report(tsan_report::none, [&] {
      exclusive_permit<cat::semaphore_binary>();
   });
}

$test(tsan_semaphore_binary_release_publishes) {
   expect_tsan_report(tsan_report::none, [&] {
      release_publishes<cat::semaphore_binary>();
   });
}

$test(tsan_semaphore_binary_failed_try_acquire) {
   expect_tsan_report(tsan_report::data_race, [&] {
      failed_acquire<cat::semaphore_binary>([](auto& semaphore) {
         return semaphore.try_acquire();
      });
   });
}

$test(tsan_semaphore_binary_timed_out_acquire) {
   expect_tsan_report(tsan_report::data_race, [&] {
      failed_acquire<cat::semaphore_binary>([](auto& semaphore) {
         return semaphore.try_acquire_for(1 * cat::units::millisecond);
      });
   });
}

#endif
