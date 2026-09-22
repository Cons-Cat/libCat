#include <cat/atomic>
#include <cat/chrono>
#include <cat/mutex>
#include <cat/mutex_native>
#include <cat/mutex_spin>
#include <cat/page_allocator>
#include <cat/thread>

#include "../unit_tests.hpp"

namespace {

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

   for (idx index; index < thread_count; ++index) {
      threads[index]
         .spawn(
            pager, 1_umi,
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
      threads[index].free(pager);
   }
   cat::verify(counter == thread_count * iteration_count);
}

template <typename Threads>
void
join_and_free(Threads& threads, idx thread_count) {
   for (idx index; index < thread_count; ++index) {
      threads[index].join().verify();
      threads[index].free(pager);
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
   worker
      .spawn(
         pager, 1_umi,
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
   worker.free(pager);

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

$test(mutex_runtime_dyn_allocator) {
   cat::dyn_allocator allocator = pager;
   cat::alloc_mutex_runtime(allocator, 1u).verify();
   cat::free_mutex_runtime(allocator);
}

$test(mutex_runtime_dyn_allocator_cfree) {
   cat::dyn_allocator allocator = pager;
   cat::alloc_mutex_runtime(allocator, 1u).verify();
   cat::cfree_mutex_runtime(allocator);
}

$test(mutex_handoff) {
   cat::alloc_mutex_runtime<cat::page_allocator>(pager, 2u).verify();
   $defer {
      cat::free_mutex_runtime<cat::page_allocator>(pager);
   };
   cat::mutex mutex;
   cat::atomic<bool> entered;
   cat::atomic<bool> acquired;
   mutex.lock();

   cat::thread worker;
   worker
      .spawn(
         pager, 1_umi,
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
   worker.free(pager);
   cat::verify(acquired.acquire());
}

$test(mutex_protected_publication) {
   cat::alloc_mutex_runtime<cat::page_allocator>(pager, 2u).verify();
   $defer {
      cat::free_mutex_runtime<cat::page_allocator>(pager);
   };
   cat::mutex mutex;
   cat::atomic<bool> ready;
   idx value = 0;
   mutex.lock();

   cat::thread worker;
   worker
      .spawn(
         pager, 1_umi,
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
   worker.free(pager);
}

$test(mutex_shared_counter) {
   cat::alloc_mutex_runtime<cat::page_allocator>(pager, 8u).verify();
   $defer {
      cat::free_mutex_runtime<cat::page_allocator>(pager);
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
   worker
      .spawn(
         pager, 1_umi,
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
   worker.free(pager);
   cat::verify(acquired.acquire());
}

$test(mutex_native_try_lock_until_times_out) {
   cat::mutex_native mutex;
   mutex.lock();

   cat::atomic<bool> finished;
   cat::thread worker;
   worker
      .spawn(
         pager, 1_umi,
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
   worker.free(pager);
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

   for (idx index; index < thread_count; ++index) {
      threads[index]
         .spawn(
            pager, 1_umi,
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
   join_and_free(threads, thread_count);
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
   mutex.lock();

   for (idx index; index < thread_count; ++index) {
      threads[index]
         .spawn(
            pager, 1_umi,
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
   join_and_free(threads, thread_count);
   cat::verify(acquired.load(cat::memory_order::acquire) == 0);
   mutex.unlock();
}

$test(mutex_mixed_try_lock_and_lock) {
   constexpr idx thread_count = 8;
   constexpr idx iteration_count = 5'000;
   cat::alloc_mutex_runtime<cat::page_allocator>(pager, thread_count).verify();
   $defer {
      cat::free_mutex_runtime<cat::page_allocator>(pager);
   };
   cat::mutex mutex;
   idx counter = 0;
   cat::thread threads[8];

   for (idx index; index < thread_count; ++index) {
      threads[index]
         .spawn(
            pager, 1_umi,
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
   join_and_free(threads, thread_count);
   cat::verify(counter == thread_count * iteration_count);
}

$test(mutex_parks_waiters) {
   constexpr idx thread_count = 16;
   cat::alloc_mutex_runtime<cat::page_allocator>(pager, thread_count).verify();
   $defer {
      cat::free_mutex_runtime<cat::page_allocator>(pager);
   };
   cat::mutex mutex;
   cat::atomic<idx> ready;
   cat::atomic<idx> acquired;
   cat::thread threads[16];
   mutex.lock();

   for (idx index; index < thread_count; ++index) {
      threads[index]
         .spawn(
            pager, 1_umi,
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
   join_and_free(threads, thread_count);
   cat::verify(acquired.load(cat::memory_order::acquire) == thread_count);
}

$test(mutex_exclusion_under_yield) {
   constexpr idx thread_count = 8;
   constexpr idx iteration_count = 2'000;
   cat::alloc_mutex_runtime<cat::page_allocator>(pager, thread_count).verify();
   $defer {
      cat::free_mutex_runtime<cat::page_allocator>(pager);
   };
   cat::mutex mutex;
   cat::atomic<idx> active;
   idx counter = 0;
   cat::thread threads[8];

   for (idx index; index < thread_count; ++index) {
      threads[index]
         .spawn(
            pager, 1_umi,
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
   join_and_free(threads, thread_count);
   cat::verify(active.load(cat::memory_order::relaxed) == 0);
   cat::verify(counter == thread_count * iteration_count);
}

$test(mutex_try_lock_for_waits) {
   cat::alloc_mutex_runtime<cat::page_allocator>(pager, 2u).verify();
   $defer {
      cat::free_mutex_runtime<cat::page_allocator>(pager);
   };
   cat::mutex mutex;
   cat::atomic<bool> entered;
   cat::atomic<bool> acquired;
   mutex.lock();

   cat::thread worker;
   worker
      .spawn(
         pager, 1_umi,
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
   worker.free(pager);
   cat::verify(acquired.acquire());
}

$test(mutex_try_lock_until_times_out) {
   cat::alloc_mutex_runtime<cat::page_allocator>(pager, 2u).verify();
   $defer {
      cat::free_mutex_runtime<cat::page_allocator>(pager);
   };
   cat::mutex mutex;
   mutex.lock();

   cat::atomic<bool> finished;
   cat::thread worker;
   worker
      .spawn(
         pager, 1_umi,
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
   worker.free(pager);
   mutex.unlock();
}

$test(mutex_try_lock_until_accepts_system_clock) {
   cat::alloc_mutex_runtime<cat::page_allocator>(pager, 1u).verify();
   $defer {
      cat::free_mutex_runtime<cat::page_allocator>(pager);
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
   worker
      .spawn(
         pager, 1_umi,
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
   worker.free(pager);
   cat::verify(acquired.acquire());
}

$test(mutex_spin_try_lock_until_times_out) {
   cat::mutex_spin mutex;
   mutex.lock();

   cat::atomic<bool> finished;
   cat::thread worker;
   worker
      .spawn(
         pager, 1_umi,
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
   worker.free(pager);
   mutex.unlock();
}

$test(mutex_unlock_fair_wakes_waiter) {
   cat::alloc_mutex_runtime<cat::page_allocator>(pager, 2u).verify();
   $defer {
      cat::free_mutex_runtime<cat::page_allocator>(pager);
   };
   cat::mutex mutex;
   cat::atomic<bool> entered;
   cat::atomic<bool> acquired;
   mutex.lock();

   cat::thread worker;
   worker
      .spawn(
         pager, 1_umi,
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
   worker.free(pager);
   cat::verify(acquired.acquire());
}

$test(mutex_yield_to_waiters) {
   cat::alloc_mutex_runtime<cat::page_allocator>(pager, 2u).verify();
   $defer {
      cat::free_mutex_runtime<cat::page_allocator>(pager);
   };
   cat::mutex mutex;
   cat::atomic<bool> entered;
   cat::atomic<bool> waiter_held;
   mutex.lock();

   cat::thread worker;
   worker
      .spawn(
         pager, 1_umi,
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
   worker.free(pager);
}

$test(mutex_two_lock_order) {
   constexpr idx thread_count = 8;
   constexpr idx iteration_count = 2'000;
   cat::alloc_mutex_runtime<cat::page_allocator>(pager, thread_count).verify();
   $defer {
      cat::free_mutex_runtime<cat::page_allocator>(pager);
   };
   cat::mutex first;
   cat::mutex second;
   idx counter = 0;
   cat::thread threads[8];

   for (idx index; index < thread_count; ++index) {
      threads[index]
         .spawn(
            pager, 1_umi,
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
   join_and_free(threads, thread_count);
   cat::verify(counter == thread_count * iteration_count);
}
