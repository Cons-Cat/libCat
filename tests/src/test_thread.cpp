#include <cat/allocator_page>
#include <cat/atomic>
#include <cat/bit>
#include <cat/linux>
#include <cat/raii_thread>
#include <cat/raii_vec>
#include <cat/runtime>
#include <cat/string>
#include <cat/thread>

#include "../unit_tests.hpp"

namespace {

constinit cat::atomic<int> atomic{};

thread_local int tls1 = 1;
thread_local int tls2 = 2;

// Each thread constructs its own on first use, and destroys it when it exits.
thread_local cat::raii::vec_big<int4, cat::allocator_musl_shared> local_values{
   g_tests_alloc
};

// These cover `.tdata`, `.tbss`, and an alignment above libCat's 32-byte
// minimum.
constinit thread_local int4 initialized_value = 7;
constinit thread_local int4 zeroed_value = 0;
alignas(128) constinit thread_local int4 aligned_value = 3;

// Whether this thread's storage starts from the `thread_local` image. Then
// overwrite it with `mark`.
auto
has_fresh_thread_locals(int4 mark) -> bool {
   // `aligned_value` lies a link-time constant below the thread pointer. The
   // compiler assumes it is aligned, so check the thread pointer instead.
   bool const is_fresh =
      initialized_value == 7 && zeroed_value == 0 && aligned_value == 3
      && __builtin_is_aligned(__builtin_thread_pointer(), 128);
   initialized_value = mark;
   zeroed_value = mark;
   aligned_value = mark;
   return is_fresh;
}

constexpr idx g_thread_test_stack_size = 16_uki;

void
function_1() {
   for (idx i = 0; i < 3; ++i) {
      ++atomic;
   }

   for (idx i = 0; i < 3; ++i) {
      ++tls1;
      ++tls2;
   }
   cat::verify(tls1 == 4);
   cat::verify(tls2 == 5);
}

void
function_2() {
   ++atomic;
}

}  // namespace

$test(this_thread) {
   cat::thread::id const original_id = cat::this_thread::get_id();
   cat::verify(original_id == cat::this_thread::get_id());

   cat::atomic<cat::uint4> observed_id{};
   cat::thread worker;
   $defer {
      worker.free(g_tests_alloc);
      cat::verify(worker.is_empty());
   };
   cat::verify(worker.is_empty());
   worker
      .spawn(
         g_tests_alloc, g_thread_test_stack_size,
         [&observed_id] {
            cat::this_thread::yield();
            observed_id.release() =
               cat::this_thread::get_id().native_handle().value;
         }
      )
      .verify();
   cat::verify(!worker.is_empty());
   cat::thread::id const worker_id = worker.get_id();
   worker.join().verify();

   cat::verify(worker_id != original_id);
   cat::verify(observed_id.acquire().load() == worker_id.native_handle().value);
}

#ifndef CAT_USE_PTHREAD
$test(thread) {
   cat::thread threads[5];
   nix::process non_thread_child;

   $defer {
      for (idx i = 0; i < 5; ++i) {
         threads[i].free(g_tests_alloc);
      }
      non_thread_child.free(g_tests_alloc);
   };

   threads[0]
      .spawn(g_tests_alloc, g_thread_test_stack_size, function_1)
      .verify();

   threads[1]
      .spawn(g_tests_alloc, g_thread_test_stack_size, &function_2)
      .verify();

   threads[2]
      .spawn(
         g_tests_alloc, g_thread_test_stack_size,
         [] {
            ++atomic;
         }
      )
      .verify();

   threads[3]
      .spawn(
         g_tests_alloc, g_thread_test_stack_size,
         [](int) {
            ++atomic;
         },
         1
      )
      .verify();

   threads[4]
      .spawn(
         g_tests_alloc, g_thread_test_stack_size,
         +[] {
            ++atomic;
         }
      )
      .verify();

   non_thread_child
      .spawn(
         g_tests_alloc, g_thread_test_stack_size,
         +[] {
            ++atomic;
         }
      )
      .verify();

   for (idx i = 0; i < 3; ++i) {
      ++atomic;
      ++tls1;
   }
   cat::verify(tls1 == 4);

   threads[0].join().verify();
   threads[1].join().verify();
   threads[2].join().verify();
   threads[3].join().verify();
   threads[4].join().verify();
   non_thread_child.wait().verify();

   cat::verify(atomic.load() == 11);
}
#endif

$test(raii_thread) {
   int4 const before = atomic.load();

   {
      cat::raii::basic_thread<cat::allocator_musl_shared> worker{g_tests_alloc};
      worker
         .spawn(
            g_thread_test_stack_size,
            [] {
               ++atomic;
            }
         )
         .verify();
      cat::verify(!worker.is_empty());
   }

   cat::verify(atomic.load() == before + 1);
}

$test(raii_thread_join) {
   int4 const before = atomic.load();

   cat::raii::basic_thread<cat::allocator_musl_shared> worker{g_tests_alloc};
   worker.spawn(g_thread_test_stack_size, &function_2).verify();
   worker.join().verify();
   worker.free();
   cat::verify(worker.is_empty());
   cat::verify(atomic.load() == before + 1);
}

$test(thread_cfree) {
   int4 const before = atomic.load();

   cat::thread worker;
   $defer {
      worker.free(g_tests_alloc);
   };
   cat::verify(worker.is_empty());
   worker.spawn(g_tests_alloc, g_thread_test_stack_size, &function_2).verify();
   cat::verify(!worker.is_empty());
   worker.join().verify();
   worker.cfree(g_tests_alloc);
   cat::verify(worker.is_empty());
   cat::verify(atomic.load() == before + 1);
}

$test(raii_thread_cfree) {
   int4 const before = atomic.load();

   cat::raii::basic_thread<cat::allocator_musl_shared> worker{g_tests_alloc};
   worker.spawn(g_thread_test_stack_size, &function_2).verify();
   worker.join().verify();
   worker.cfree();
   cat::verify(worker.is_empty());
   cat::verify(atomic.load() == before + 1);
}

#ifndef CAT_USE_PTHREAD
$test(thread_clone_failure) {
   nix::rlimit original{};
   cat::verify(
      nix::sys_getrlimit(
         nix::rlimit_resource::max_processes_and_threads_per_real_user, original
      )
         .has_value()
   );

   // Lower only the soft limit so the hard ceiling stays high enough to restore
   // `original` without `linux_error::perm` after the experiment.
   nix::rlimit const narrow{
      .soft = 1u,
      .hard = original.hard,
   };
   cat::verify(
      nix::sys_setrlimit(
         nix::rlimit_resource::max_processes_and_threads_per_real_user, narrow
      )
         .has_value()
   );

   // `cat::thread::spawn` maps errors to `nullopt`. `nix::process` with the
   // same clone flags keeps the errno from `clone` on failure.
   nix::process child;
   $defer {
      child.free(g_tests_alloc);
   };
   child.add_clone_flag(nix::clone_flags::thread);
   child.add_clone_flag(nix::clone_flags::child_set_tid);
   nix::scaredy_nix<void> const spawn_result =
      child.spawn(g_tests_alloc, g_thread_test_stack_size, [] {
      });

   cat::verify(
      nix::sys_setrlimit(
         nix::rlimit_resource::max_processes_and_threads_per_real_user, original
      )
         .has_value()
   );

   cat::verify(spawn_result.is_empty());
   cat::verify(spawn_result.error() == nix::linux_error::again);
}
#endif

$test(thread_local_vec) {
   local_values.push_back(1).verify();
   local_values.push_back(2).verify();

   idx worker_size_before = 1u;
   idx worker_size_after = 0u;
   int4 worker_sum = 0;
   cat::thread worker;
   $defer {
      worker.free(g_tests_alloc);
   };
   worker
      .spawn(
         // In Debug, `g_tests_alloc` needs about 40 KiB of stack when it
         // allocates a new group.
         g_tests_alloc, 64_uki,
         [&] {
            worker_size_before = local_values.size();
            for (int4 i = 0; i < 100; ++i) {
               local_values.push_back(i).verify();
            }
            worker_size_after = local_values.size();
            for (int4 value : local_values) {
               worker_sum += value;
            }
         }
      )
      .verify();
   worker.join().verify();

   // The worker's vector started empty, and growing it did not touch this one.
   cat::verify(worker_size_before == 0u);
   cat::verify(worker_size_after == 100u);
   cat::verify(worker_sum == 4'950);
   cat::verify(local_values.size() == 2u);
   cat::verify(local_values[0u] == 1 && local_values[1u] == 2);
}

#ifndef CAT_USE_PTHREAD
$test(thread_local_storage_per_thread) {
   cat::verify(has_fresh_thread_locals(1));

   // Stack sizes 32 bytes apart give thread pointers of every alignment that
   // libCat's 32-byte minimum allows, so `aligned_value` is only aligned if
   // the thread pointer is.
   for (idx i = 0u; i < 4u; ++i) {
      cat::atomic<bool> is_fresh{};
      cat::thread worker;
      $defer {
         worker.free(g_tests_alloc);
      };
      worker
         .spawn(
            g_tests_alloc, g_thread_test_stack_size + 32u * i,
            [&] {
               is_fresh.release() = has_fresh_thread_locals(2);
            }
         )
         .verify();
      worker.join().verify();
      cat::verify(is_fresh.acquire().load());
   }

   cat::atomic<bool> non_thread_is_fresh{};
   // This shares memory without being a thread.
   nix::process non_thread_child;
   $defer {
      non_thread_child.free(g_tests_alloc);
   };
   non_thread_child
      .spawn(
         g_tests_alloc, g_thread_test_stack_size,
         [&] {
            non_thread_is_fresh.release() = has_fresh_thread_locals(3);
         }
      )
      .verify();
   non_thread_child.wait().verify();
   cat::verify(non_thread_is_fresh.acquire().load());

   // Each child started from the image rather than this thread's values, and
   // its writes stayed in its own storage.
   cat::verify(
      initialized_value == 1 && zeroed_value == 1 && aligned_value == 1
   );
}
#endif
