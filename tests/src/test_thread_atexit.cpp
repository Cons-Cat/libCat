#include <cat/allocator_page>
#include <cat/atomic>
#include <cat/mutex>
#include <cat/runtime>
#include <cat/thread>

#include "../unit_tests.hpp"

namespace {

constexpr idx worker_stack_size = 16_uki;

constinit cat::atomic<int4> destroyed_markers{};
constinit cat::atomic<int4> recorded_order{};
constinit cat::atomic<idx> bulk_destructors{};

struct marker {
   ~marker() {
      ++destroyed_markers;
   }

   int4 touches = 0;
};

thread_local marker local_marker;

void
touch_local_marker() {
   ++local_marker.touches;
   cat::verify(local_marker.touches == 1);
}

int4 first_tag = 1;
int4 second_tag = 2;
int4 third_tag = 3;

void
record_tag(void* p_tag) {
   int4 const tag = *static_cast<int4*>(p_tag);
   recorded_order.store(
      recorded_order.load(cat::memory_order::relaxed) * 10 + tag,
      cat::memory_order::relaxed
   );
}

void
record_bulk_destructor(void* /*unused*/) {
   ++bulk_destructors;
}

}  // namespace

$test(thread_atexit_worker_threads) {
   int4 const before = destroyed_markers.load();
   constexpr idx thread_count = 4;
   cat::thread threads[thread_count];
   $defer {
      for (idx index; index < thread_count; ++index) {
         threads[index].free(g_tests_alloc);
      }
   };

   for (idx index; index < thread_count; ++index) {
      threads[index]
         .spawn(g_tests_alloc, worker_stack_size, touch_local_marker)
         .verify();
   }
   for (idx index; index < thread_count; ++index) {
      threads[index].join().verify();
   }

   cat::verify(destroyed_markers.load() == before + 4);
}

$test(thread_atexit_runs_last_in_first_out) {
   recorded_order.store(0, cat::memory_order::relaxed);

   cat::thread worker;
   $defer {
      worker.free(g_tests_alloc);
   };
   worker
      .spawn(
         g_tests_alloc, worker_stack_size,
         [] {
            cat::verify(
               cat::__cxa_thread_atexit_impl(record_tag, &first_tag, nullptr)
               == 0
            );
            cat::verify(
               cat::__cxa_thread_atexit(record_tag, &second_tag, nullptr) == 0
            );
            cat::verify(
               cat::__cxa_thread_atexit(record_tag, &third_tag, nullptr) == 0
            );
         }
      )
      .verify();
   worker.join().verify();

   cat::verify(recorded_order.load() == 321);
}

$test(thread_atexit_grows_destructor_list) {
   bulk_destructors.store(0, cat::memory_order::relaxed);

   cat::thread worker;
   $defer {
      worker.free(g_tests_alloc);
   };
   worker
      .spawn(
         g_tests_alloc, worker_stack_size,
         [] {
            for (idx index; index < 300; ++index) {
               cat::verify(
                  cat::__cxa_thread_atexit(
                     record_bulk_destructor, nullptr, nullptr
                  )
                  == 0
               );
            }
         }
      )
      .verify();
   worker.join().verify();

   cat::verify(bulk_destructors.load() == 300);
}

$test(thread_atexit_parking_lot_threads_exit) {
   constexpr idx thread_count = 4;
   cat::alloc_mutex_runtime<cat::allocator_musl_shared>(
      g_tests_alloc, thread_count
   )
      .verify();

   cat::mutex mutex;
   cat::atomic<idx> entered;
   cat::thread threads[thread_count];
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
               ++entered;
               mutex.lock();
               mutex.unlock();
            }
         )
         .verify();
   }
   while (entered.acquire() != thread_count) {
      cat::machine_pause();
   }
   for (idx iteration; iteration < 1'000; ++iteration) {
      cat::this_thread::yield();
   }
   mutex.unlock();
   for (idx index; index < thread_count; ++index) {
      threads[index].join().verify();
   }

   cat::free_mutex_runtime<cat::allocator_musl_shared>(g_tests_alloc);
}
