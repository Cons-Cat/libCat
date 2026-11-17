#include <cat/allocator_inplace>
#include <cat/allocator_page>
#include <cat/atomic>
#include <cat/chrono>
#include <cat/mutex>
#include <cat/mutex_native>
#include <cat/mutex_spin>
#include <cat/thread>

#include "../tsan_tests.hpp"
#include "../unit_tests.hpp"

// The parking lot's hash table cannot grow, so a known number of threads can
// back it with static storage, as `<cat/mutex>` recommends. No test here uses
// more than 16 threads, whose hash table takes about 4 KiB.
inline cat::allocator_inplace<5_uki> g_mutex_runtime_allocator;

namespace {

constexpr idx worker_stack_size = 16_uki;

struct not_mutex {
   void
   lock() {
   }

   void
   unlock() {
   }
};

struct mutex_without_timed_waits {
   void
   lock() {
   }

   auto
   try_lock() -> bool {
      return true;
   }

   void
   unlock() {
   }
};

static_assert(cat::is_mutex<cat::mutex>);
static_assert(cat::is_mutex<cat::mutex_native>);
static_assert(cat::is_mutex<cat::mutex_spin>);
static_assert(!cat::is_mutex<not_mutex>);
static_assert(!cat::is_mutex<mutex_without_timed_waits>);
static_assert(cat::is_mutex_fair<cat::mutex>);
static_assert(!cat::is_mutex_fair<cat::mutex_native>);
static_assert(!cat::is_mutex_fair<cat::mutex_spin>);
static_assert(cat::is_mutex<cat::mutex_fair<cat::mutex>>);
static_assert(cat::is_mutex<cat::mutex_fair<>>);
static_assert(cat::is_mutex_fair<cat::mutex_fair<cat::mutex>>);
static_assert(cat::is_mutex_fair<cat::mutex_fair<>>);
static_assert(cat::is_same<
              decltype(cat::mutex_fair{cat::declval<cat::mutex&>()}),
              cat::mutex_fair<cat::mutex>>);

template <typename Mutex>
void
verify_counter() {
   constexpr idx thread_count = 8;
   constexpr idx iteration_count = 10'000;
   Mutex mutex;
   idx counter = 0;

   cat::thread threads[8];
   $defer {
      for (idx index; index < thread_count; ++index) {
         threads[index].free(g_tests_alloc);
      }
   };

   for (idx index; index < thread_count; ++index) {
      threads[index]
         .spawn(
            g_tests_alloc, worker_stack_size,
            [&mutex, &counter, iteration_count] {
               for (idx iteration; iteration < iteration_count; ++iteration) {
                  mutex.lock();
                  ++counter;
                  mutex.unlock();
               }
            }
         )
         .verify();
   }

   for (idx index; index < thread_count; ++index) {
      threads[index].join().verify();
   }
   cat::verify(counter == thread_count * iteration_count);
}

template <typename Threads>
void
join_threads(Threads& threads, idx thread_count) {
   for (idx index; index < thread_count; ++index) {
      threads[index].join().verify();
   }
}

}  // namespace

$test(mutex_lock_unlock) {
   cat::mutex mutex;
   mutex.lock();
   mutex.unlock();

   constexpr idx iteration_count = 100'000;
   for (idx iteration; iteration < iteration_count; ++iteration) {
      mutex.lock();
      mutex.unlock();
   }
}

$test(mutex_fair_lock_unlock) {
   cat::mutex mutex;
   cat::mutex_fair fair_mutex(mutex);
   fair_mutex.lock();
   cat::verify(!fair_mutex.try_lock());
   fair_mutex.unlock();
   cat::verify(fair_mutex.try_lock());
   fair_mutex.unlock();
}

$test(mutex_try_lock) {
   cat::mutex mutex;
   cat::verify(mutex.try_lock());
   mutex.unlock();

   cat::atomic<bool> attempted;
   cat::atomic<bool> acquired;
   mutex.lock();

   cat::thread worker;
   $defer {
      worker.free(g_tests_alloc);
   };

   worker
      .spawn(
         g_tests_alloc, worker_stack_size,
         [&] {
            acquired.store(mutex.try_lock(), cat::memory_order::release);
            if (acquired.load(cat::memory_order::relaxed)) {
               mutex.unlock();
            }
            attempted.store(true, cat::memory_order::release);
         }
      )
      .verify();

   while (!attempted.load(cat::memory_order::acquire)) {
      cat::machine_pause();
   }

   cat::verify(!acquired.load(cat::memory_order::acquire));
   worker.join().verify();

   mutex.unlock();
   cat::verify(mutex.try_lock());
   mutex.unlock();
}

$test(mutex_destroy_unlocked) {
   {
      cat::mutex mutex;
   }
   {
      cat::mutex mutex;
      mutex.lock();
      mutex.unlock();
   }
}

$test(mutex_runtime_allocator_dyn) {
   cat::allocator_dyn allocator = g_tests_alloc;
   cat::alloc_mutex_runtime(allocator, 1u).verify();
   cat::free_mutex_runtime(allocator);
}

$test(mutex_runtime_allocator_dyn_cfree) {
   cat::allocator_dyn allocator = g_tests_alloc;
   cat::alloc_mutex_runtime(allocator, 1u).verify();
   cat::cfree_mutex_runtime(allocator);
}

$test(mutex_handoff) {
   cat::alloc_mutex_runtime(g_mutex_runtime_allocator, 2u).verify();
   $defer {
      cat::free_mutex_runtime(g_mutex_runtime_allocator);
   };

   cat::mutex mutex;
   cat::atomic<bool> entered;
   cat::atomic<bool> acquired;
   mutex.lock();

   cat::thread worker;
   $defer {
      worker.free(g_tests_alloc);
   };

   worker
      .spawn(
         g_tests_alloc, worker_stack_size,
         [&] {
            entered.release() = true;
            mutex.lock();
            acquired.release() = true;
            mutex.unlock();
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
   mutex.unlock();
   worker.join().verify();
   cat::verify(acquired.acquire());
}

$test(mutex_protected_publication) {
   cat::alloc_mutex_runtime<cat::allocator_inplace<5_uki>>(
      g_mutex_runtime_allocator, 2u
   )
      .verify();
   $defer {
      cat::free_mutex_runtime<cat::allocator_inplace<5_uki>>(
         g_mutex_runtime_allocator
      );
   };

   cat::mutex mutex;
   cat::atomic<bool> ready;
   idx value = 0;
   mutex.lock();

   cat::thread worker;
   $defer {
      worker.free(g_tests_alloc);
   };

   worker
      .spawn(
         g_tests_alloc, worker_stack_size,
         [&] {
            ready.store(true, cat::memory_order::release);
            mutex.lock();
            cat::verify(value == 42);
            mutex.unlock();
         }
      )
      .verify();
   while (!ready.load(cat::memory_order::acquire)) {
      cat::machine_pause();
   }
   value = 42;
   mutex.unlock();
   worker.join().verify();
}

$test(mutex_shared_counter) {
   cat::alloc_mutex_runtime<cat::allocator_inplace<5_uki>>(
      g_mutex_runtime_allocator, 8u
   )
      .verify();
   $defer {
      cat::free_mutex_runtime<cat::allocator_inplace<5_uki>>(
         g_mutex_runtime_allocator
      );
   };

   verify_counter<cat::mutex>();
}

$test(mutex_spin_shared_counter) {
   verify_counter<cat::mutex_spin>();
}

$test(mutex_native_lock_try_unlock) {
   cat::mutex_native mutex;
   mutex.lock();
   cat::verify(!mutex.try_lock());
   mutex.unlock();
   cat::verify(mutex.try_lock());
   mutex.unlock();
}

$test(mutex_native_shared_counter) {
   verify_counter<cat::mutex_native>();
}

$test(mutex_native_try_lock_for_waits) {
   cat::mutex_native mutex;
   cat::atomic<bool> entered;
   cat::atomic<bool> acquired;
   mutex.lock();

   cat::thread worker;
   $defer {
      worker.free(g_tests_alloc);
   };

   worker
      .spawn(
         g_tests_alloc, worker_stack_size,
         [&] {
            entered.release() = true;
            cat::verify(mutex.try_lock_for(1 * cat::units::second));
            acquired.release() = true;
            mutex.unlock();
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
   mutex.unlock();
   worker.join().verify();
   cat::verify(acquired.acquire());
}

$test(mutex_native_try_lock_until_times_out) {
   cat::mutex_native mutex;
   mutex.lock();

   cat::atomic<bool> finished;
   cat::thread worker;
   $defer {
      worker.free(g_tests_alloc);
   };

   worker
      .spawn(
         g_tests_alloc, worker_stack_size,
         [&] {
            cat::clock_steady::time_point const deadline =
               cat::clock_steady::now().verify() + 10 * cat::units::millisecond;
            cat::verify(!mutex.try_lock_until(deadline));
            finished.release() = true;
         }
      )
      .verify();

   while (!finished.acquire()) {
      cat::machine_pause();
   }

   worker.join().verify();
   mutex.unlock();
}

$test(mutex_concurrent_try_lock_single_winner) {
   constexpr idx thread_count = 16;
   cat::mutex mutex;
   cat::atomic<idx> ready;
   cat::atomic<bool> start;
   cat::atomic<idx> attempted;
   cat::atomic<idx> acquired;

   cat::thread threads[16];
   $defer {
      for (idx index; index < thread_count; ++index) {
         threads[index].free(g_tests_alloc);
      }
   };

   for (idx index; index < thread_count; ++index) {
      threads[index]
         .spawn(
            g_tests_alloc, worker_stack_size,
            [&] {
               ready.fetch_add(1, cat::memory_order::release);
               while (!start.load(cat::memory_order::acquire)) {
                  cat::machine_pause();
               }

               bool const owns_lock = mutex.try_lock();
               attempted.fetch_add(1, cat::memory_order::release);

               if (owns_lock) {
                  acquired.fetch_add(1, cat::memory_order::relaxed);
                  while (attempted.load(cat::memory_order::acquire)
                         != thread_count) {
                     cat::machine_pause();
                  }
                  mutex.unlock();
               }
            }
         )
         .verify();
   }

   while (ready.load(cat::memory_order::acquire) != thread_count) {
      cat::machine_pause();
   }

   start.store(true, cat::memory_order::release);
   join_threads(threads, thread_count);
   cat::verify(attempted.load(cat::memory_order::acquire) == thread_count);
   cat::verify(acquired.load(cat::memory_order::acquire) == 1);
}

$test(mutex_try_lock_fails_for_all_waiters) {
   constexpr idx thread_count = 16;
   cat::mutex mutex;
   cat::atomic<idx> ready;
   cat::atomic<bool> start;
   cat::atomic<idx> acquired;

   cat::thread threads[16];
   $defer {
      for (idx index; index < thread_count; ++index) {
         threads[index].free(g_tests_alloc);
      }
   };
   mutex.lock();

   for (idx index; index < thread_count; ++index) {
      threads[index]
         .spawn(
            g_tests_alloc, worker_stack_size,
            [&] {
               ready.fetch_add(1, cat::memory_order::release);
               while (!start.load(cat::memory_order::acquire)) {
                  cat::machine_pause();
               }
               if (mutex.try_lock()) {
                  acquired.fetch_add(1, cat::memory_order::relaxed);
                  mutex.unlock();
               }
            }
         )
         .verify();
   }

   while (ready.load(cat::memory_order::acquire) != thread_count) {
      cat::machine_pause();
   }

   start.store(true, cat::memory_order::release);
   join_threads(threads, thread_count);
   cat::verify(acquired.load(cat::memory_order::acquire) == 0);
   mutex.unlock();
}

$test(mutex_mixed_try_lock_and_lock) {
   constexpr idx thread_count = 8;
   constexpr idx iteration_count = 5'000;
   cat::alloc_mutex_runtime<cat::allocator_inplace<5_uki>>(
      g_mutex_runtime_allocator, thread_count
   )
      .verify();
   $defer {
      cat::free_mutex_runtime<cat::allocator_inplace<5_uki>>(
         g_mutex_runtime_allocator
      );
   };

   cat::mutex mutex;
   idx counter = 0;

   cat::thread threads[8];
   $defer {
      for (idx index; index < thread_count; ++index) {
         threads[index].free(g_tests_alloc);
      }
   };

   for (idx index; index < thread_count; ++index) {
      threads[index]
         .spawn(
            g_tests_alloc, worker_stack_size,
            [&, index] {
               for (idx iteration; iteration < iteration_count; ++iteration) {
                  bool const use_try_lock = (index + iteration) % 2u != 0u;
                  if (!use_try_lock || !mutex.try_lock()) {
                     mutex.lock();
                  }
                  ++counter;
                  mutex.unlock();
               }
            }
         )
         .verify();
   }

   join_threads(threads, thread_count);
   cat::verify(counter == thread_count * iteration_count);
}

$test(mutex_parks_waiters) {
   constexpr idx thread_count = 16;
   cat::alloc_mutex_runtime<cat::allocator_inplace<5_uki>>(
      g_mutex_runtime_allocator, thread_count
   )
      .verify();
   $defer {
      cat::free_mutex_runtime<cat::allocator_inplace<5_uki>>(
         g_mutex_runtime_allocator
      );
   };

   cat::mutex mutex;
   cat::atomic<idx> ready;
   cat::atomic<idx> acquired;

   cat::thread threads[16];
   $defer {
      for (idx index; index < thread_count; ++index) {
         threads[index].free(g_tests_alloc);
      }
   };

   mutex.lock();

   for (idx index; index < thread_count; ++index) {
      threads[index]
         .spawn(
            g_tests_alloc, worker_stack_size,
            [&] {
               ready.fetch_add(1, cat::memory_order::release);
               mutex.lock();
               acquired.fetch_add(1, cat::memory_order::release);
               mutex.unlock();
            }
         )
         .verify();
   }

   while (ready.load(cat::memory_order::acquire) != thread_count) {
      cat::machine_pause();
   }

   for (idx iteration; iteration < 2'000; ++iteration) {
      cat::this_thread::yield();
   }
   cat::verify(acquired.load(cat::memory_order::acquire) == 0);
   mutex.unlock();
   join_threads(threads, thread_count);
   cat::verify(acquired.load(cat::memory_order::acquire) == thread_count);
}

$test(mutex_exclusion_under_yield) {
   constexpr idx thread_count = 8;
   constexpr idx iteration_count = 2'000;

   cat::alloc_mutex_runtime<cat::allocator_inplace<5_uki>>(
      g_mutex_runtime_allocator, thread_count
   )
      .verify();
   $defer {
      cat::free_mutex_runtime<cat::allocator_inplace<5_uki>>(
         g_mutex_runtime_allocator
      );
   };

   cat::mutex mutex;
   cat::atomic<idx> active;
   idx counter = 0;

   cat::thread threads[8];
   $defer {
      for (idx index; index < thread_count; ++index) {
         threads[index].free(g_tests_alloc);
      }
   };

   for (idx index; index < thread_count; ++index) {
      threads[index]
         .spawn(
            g_tests_alloc, worker_stack_size,
            [&] {
               for (idx iteration; iteration < iteration_count; ++iteration) {
                  mutex.lock();
                  cat::verify(
                     active.fetch_add(1, cat::memory_order::relaxed) == 0
                  );
                  cat::this_thread::yield();
                  ++counter;
                  cat::verify(
                     active.fetch_sub(1, cat::memory_order::relaxed) == 1
                  );
                  mutex.unlock();
                  cat::this_thread::yield();
               }
            }
         )
         .verify();
   }

   join_threads(threads, thread_count);
   cat::verify(active.load(cat::memory_order::relaxed) == 0);
   cat::verify(counter == thread_count * iteration_count);
}

$test(mutex_try_lock_for_waits) {
   cat::alloc_mutex_runtime<cat::allocator_inplace<5_uki>>(
      g_mutex_runtime_allocator, 2u
   )
      .verify();
   $defer {
      cat::free_mutex_runtime<cat::allocator_inplace<5_uki>>(
         g_mutex_runtime_allocator
      );
   };

   cat::mutex mutex;
   cat::atomic<bool> entered;
   cat::atomic<bool> acquired;
   mutex.lock();

   cat::thread worker;
   $defer {
      worker.free(g_tests_alloc);
   };

   worker
      .spawn(
         g_tests_alloc, worker_stack_size,
         [&] {
            entered.release() = true;
            cat::verify(mutex.try_lock_for(1 * cat::units::second));
            acquired.release() = true;
            mutex.unlock();
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
   mutex.unlock();
   worker.join().verify();
   cat::verify(acquired.acquire());
}

$test(mutex_try_lock_until_times_out) {
   cat::alloc_mutex_runtime<cat::allocator_inplace<5_uki>>(
      g_mutex_runtime_allocator, 2u
   )
      .verify();
   $defer {
      cat::free_mutex_runtime<cat::allocator_inplace<5_uki>>(
         g_mutex_runtime_allocator
      );
   };

   cat::mutex mutex;
   mutex.lock();

   cat::atomic<bool> finished;
   cat::thread worker;
   $defer {
      worker.free(g_tests_alloc);
   };

   worker
      .spawn(
         g_tests_alloc, worker_stack_size,
         [&] {
            cat::clock_steady::time_point const deadline =
               cat::clock_steady::now().verify() + 10 * cat::units::millisecond;
            cat::verify(!mutex.try_lock_until(deadline));
            finished.release() = true;
         }
      )
      .verify();

   while (!finished.acquire()) {
      cat::machine_pause();
   }

   worker.join().verify();
   mutex.unlock();
}

$test(mutex_try_lock_until_accepts_system_clock) {
   cat::alloc_mutex_runtime<cat::allocator_inplace<5_uki>>(
      g_mutex_runtime_allocator, 1u
   )
      .verify();
   $defer {
      cat::free_mutex_runtime<cat::allocator_inplace<5_uki>>(
         g_mutex_runtime_allocator
      );
   };

   cat::mutex mutex;
   mutex.lock();

   cat::clock_system::time_point const deadline =
      cat::clock_system::now().verify() + 10 * cat::units::millisecond;
   cat::verify(!mutex.try_lock_until(deadline));
   mutex.unlock();
}

$test(mutex_spin_try_lock_for_waits) {
   cat::mutex_spin mutex;
   cat::atomic<bool> entered;
   cat::atomic<bool> acquired;
   mutex.lock();

   cat::thread worker;
   $defer {
      worker.free(g_tests_alloc);
   };

   worker
      .spawn(
         g_tests_alloc, worker_stack_size,
         [&] {
            entered.release() = true;
            cat::verify(mutex.try_lock_for(1 * cat::units::second));
            acquired.release() = true;
            mutex.unlock();
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
   mutex.unlock();
   worker.join().verify();
   cat::verify(acquired.acquire());
}

$test(mutex_spin_try_lock_until_times_out) {
   cat::mutex_spin mutex;
   mutex.lock();

   cat::atomic<bool> finished;
   cat::thread worker;
   $defer {
      worker.free(g_tests_alloc);
   };

   worker
      .spawn(
         g_tests_alloc, worker_stack_size,
         [&] {
            cat::clock_steady::time_point const deadline =
               cat::clock_steady::now().verify() + 10 * cat::units::millisecond;
            cat::verify(!mutex.try_lock_until(deadline));
            finished.release() = true;
         }
      )
      .verify();

   while (!finished.acquire()) {
      cat::machine_pause();
   }

   worker.join().verify();
   mutex.unlock();
}

$test(mutex_unlock_fair_wakes_waiter) {
   cat::alloc_mutex_runtime<cat::allocator_inplace<5_uki>>(
      g_mutex_runtime_allocator, 2u
   )
      .verify();
   $defer {
      cat::free_mutex_runtime<cat::allocator_inplace<5_uki>>(
         g_mutex_runtime_allocator
      );
   };

   cat::mutex mutex;
   cat::atomic<bool> entered;
   cat::atomic<bool> acquired;
   mutex.lock();

   cat::thread worker;
   $defer {
      worker.free(g_tests_alloc);
   };

   worker
      .spawn(
         g_tests_alloc, worker_stack_size,
         [&] {
            entered.release() = true;
            mutex.lock();
            acquired.release() = true;
            mutex.unlock();
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
   mutex.unlock_fair();
   worker.join().verify();
   cat::verify(acquired.acquire());
}

$test(mutex_yield_to_waiters) {
   cat::alloc_mutex_runtime<cat::allocator_inplace<5_uki>>(
      g_mutex_runtime_allocator, 2u
   )
      .verify();
   $defer {
      cat::free_mutex_runtime<cat::allocator_inplace<5_uki>>(
         g_mutex_runtime_allocator
      );
   };

   cat::mutex mutex;
   cat::atomic<bool> entered;
   cat::atomic<bool> waiter_held;
   mutex.lock();

   cat::thread worker;
   $defer {
      worker.free(g_tests_alloc);
   };

   worker
      .spawn(
         g_tests_alloc, worker_stack_size,
         [&] {
            entered.release() = true;
            mutex.lock();
            waiter_held.release() = true;
            mutex.unlock();
         }
      )
      .verify();

   while (!entered.acquire()) {
      cat::machine_pause();
   }

   for (idx iteration; iteration < 1'000; ++iteration) {
      cat::this_thread::yield();
   }

   cat::verify(!waiter_held.acquire());
   mutex.yield_to_waiters();
   cat::verify(waiter_held.acquire());
   mutex.unlock();
   worker.join().verify();
}

$test(mutex_two_lock_order) {
   constexpr idx thread_count = 8;
   constexpr idx iteration_count = 2'000;
   cat::alloc_mutex_runtime<cat::allocator_inplace<5_uki>>(
      g_mutex_runtime_allocator, thread_count
   )
      .verify();
   $defer {
      cat::free_mutex_runtime<cat::allocator_inplace<5_uki>>(
         g_mutex_runtime_allocator
      );
   };

   cat::mutex first;
   cat::mutex second;
   idx counter = 0;

   cat::thread threads[8];
   $defer {
      for (idx index; index < thread_count; ++index) {
         threads[index].free(g_tests_alloc);
      }
   };

   for (idx index; index < thread_count; ++index) {
      threads[index]
         .spawn(
            g_tests_alloc, worker_stack_size,
            [&] {
               for (idx iteration; iteration < iteration_count; ++iteration) {
                  first.lock();
                  second.lock();
                  ++counter;
                  second.unlock();
                  first.unlock();
               }
            }
         )
         .verify();
   }

   join_threads(threads, thread_count);
   cat::verify(counter == thread_count * iteration_count);
}

// These verify that ThreadSanitizer reports races, so they only run with it.
#if __has_feature(thread_sanitizer)

namespace {

template <typename Mutex>
void
with_mutex(auto&& callback) {
   if constexpr (cat::is_same<Mutex, cat::mutex>) {
      cat::alloc_mutex_runtime<cat::allocator_inplace<5_uki>>(
         g_mutex_runtime_allocator, 2u
      )
         .verify();
   }

   $defer {
      if constexpr (cat::is_same<Mutex, cat::mutex>) {
         cat::free_mutex_runtime<cat::allocator_inplace<5_uki>>(
            g_mutex_runtime_allocator
         );
      }
   };

   Mutex mutex;
   callback(mutex);
}

// Two threads increment `counter`, but only one of them locks `mutex` when
// `is_both_locked` is `false`.
template <typename Mutex>
void
counter(bool is_both_locked) {
   with_mutex<Mutex>([&](Mutex& mutex) {
      alignas(64) idx counter = 0;
      nix::futex incremented{};
      nix::futex finished{};

      cat::thread worker;
      $defer {
         worker.free(g_tests_alloc);
      };

      worker
         .spawn(
            g_tests_alloc, tsan_stack_size,
            [&] {
               unsynchronized_barrier(incremented, 2u);
               mutex.lock();
               ++counter;
               mutex.unlock();
               unsynchronized_barrier(finished, 2u);
            }
         )
         .verify();

      if (is_both_locked) {
         mutex.lock();
      }
      ++counter;
      if (is_both_locked) {
         mutex.unlock();
      }
      unsynchronized_barrier(incremented, 2u);
      unsynchronized_barrier(finished, 2u);
      worker.join().verify();
   });
}

// Lock two mutexes in both orders. ThreadSanitizer reports the cycle without
// needing it to deadlock.
template <typename Mutex>
void
lock_order_inversion() {
   with_mutex<Mutex>([&](Mutex& first) {
      Mutex second;
      first.lock();
      second.lock();
      second.unlock();
      first.unlock();

      second.lock();
      first.lock();
      first.unlock();
      second.unlock();
   });
}

}  // namespace

$test(tsan_mutex_unlocked_access) {
   expect_tsan_report(tsan_report::data_race, [&] {
      counter<cat::mutex>(false);
   });
}

$test(tsan_mutex_locked_access) {
   expect_tsan_report(tsan_report::none, [&] {
      counter<cat::mutex>(true);
   });
}

$test(tsan_mutex_lock_order_inversion) {
   expect_tsan_report(tsan_report::lock_order_inversion, [&] {
      lock_order_inversion<cat::mutex>();
   });
}

$test(tsan_mutex_native_unlocked_access) {
   expect_tsan_report(tsan_report::data_race, [&] {
      counter<cat::mutex_native>(false);
   });
}

$test(tsan_mutex_native_locked_access) {
   expect_tsan_report(tsan_report::none, [&] {
      counter<cat::mutex_native>(true);
   });
}

$test(tsan_mutex_native_lock_order_inversion) {
   expect_tsan_report(tsan_report::lock_order_inversion, [&] {
      lock_order_inversion<cat::mutex_native>();
   });
}

$test(tsan_mutex_spin_unlocked_access) {
   expect_tsan_report(tsan_report::data_race, [&] {
      counter<cat::mutex_spin>(false);
   });
}

$test(tsan_mutex_spin_locked_access) {
   expect_tsan_report(tsan_report::none, [&] {
      counter<cat::mutex_spin>(true);
   });
}

$test(tsan_mutex_spin_lock_order_inversion) {
   expect_tsan_report(tsan_report::lock_order_inversion, [&] {
      lock_order_inversion<cat::mutex_spin>();
   });
}

// `cat::mutex_fair` forwards to its mutex, so it must keep its annotations.
$test(tsan_mutex_fair_unlocked_access) {
   expect_tsan_report(tsan_report::data_race, [&] {
      with_mutex<cat::mutex>([&](cat::mutex& mutex) {
         cat::mutex_fair fair_mutex(mutex);
         alignas(64) idx counter = 0;
         nix::futex incremented{};
         nix::futex finished{};

         cat::thread worker;
         $defer {
            worker.free(g_tests_alloc);
         };

         worker
            .spawn(
               g_tests_alloc, tsan_stack_size,
               [&] {
                  unsynchronized_barrier(incremented, 2u);
                  fair_mutex.lock();
                  ++counter;
                  fair_mutex.unlock();
                  unsynchronized_barrier(finished, 2u);
               }
            )
            .verify();

         ++counter;
         unsynchronized_barrier(incremented, 2u);
         unsynchronized_barrier(finished, 2u);
         worker.join().verify();
      });
   });
}

#endif
