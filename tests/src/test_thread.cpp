#include <cat/atomic>
#include <cat/bit>
#include <cat/linux>
#include <cat/page_allocator>
#include <cat/raii_thread>
#include <cat/runtime>
#include <cat/string>
#include <cat/thread>

#include "../unit_tests.hpp"

namespace {

// NOLINTNEXTLINE
constinit cat::atomic<int> atomic{};

thread_local int tls1 = 1;
thread_local int tls2 = 2;

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

$test(thread) {
   cat::thread threads[5];
   nix::process non_thread_child;

   $defer {
      for (idx i = 0; i < 5; ++i) {
         threads[i].free(pager);
      }
      non_thread_child.free(pager);
   };

   threads[0].spawn(pager, 2_uki, function_1).verify();

   threads[1].spawn(pager, 2_uki, &function_2).verify();

   threads[2]
      .spawn(
         pager, 2_uki,
         [] {
            ++atomic;
         }
      )
      .verify();

   threads[3]
      .spawn(
         pager, 2_uki,
         [](int) {
            ++atomic;
         },
         1
      )
      .verify();

   threads[4]
      .spawn(
         pager, 2_uki,
         +[] {
            ++atomic;
         }
      )
      .verify();

   non_thread_child
      .spawn(
         pager, 2_uki,
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

$test(raii_thread) {
   int4 const before = atomic.load();

   {
      cat::raii::basic_thread<cat::page_allocator> worker{pager};
      worker
         .spawn(
            2_uki,
            [] {
               ++atomic;
            }
         )
         .verify();
      cat::verify(worker.has_stack());
   }

   cat::verify(atomic.load() == before + 1);
}

$test(raii_thread_join) {
   int4 const before = atomic.load();

   cat::raii::basic_thread<cat::page_allocator> worker{pager};
   worker.spawn(2_uki, &function_2).verify();
   worker.join().verify();
   worker.free();
   cat::verify(!worker.has_stack());
   cat::verify(atomic.load() == before + 1);
}

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
   child.add_clone_flag(nix::clone_flags::thread);
   child.add_clone_flag(nix::clone_flags::child_set_tid);
   nix::scaredy_nix<void> const spawn_result = child.spawn(pager, 2_uki, [] {
   });

   cat::verify(
      nix::sys_setrlimit(
         nix::rlimit_resource::max_processes_and_threads_per_real_user, original
      )
         .has_value()
   );

   cat::verify(!spawn_result.has_value());
   cat::verify(spawn_result.error() == nix::linux_error::again);
}
