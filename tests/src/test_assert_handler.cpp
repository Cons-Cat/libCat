#include <cat/atomic>
#include <cat/maybe>
#include <cat/page_allocator>
#include <cat/thread>

#include "../unit_tests.hpp"

namespace {

constinit cat::idx block_hits{};
constinit cat::idx inner_hits{};
constinit cat::idx thread_hits{};
constinit cat::idx global_hits{};
constinit cat::idx explicit_hits{};
constinit cat::atomic<cat::idx> worker_fallback_hits{};

void
block_handler(cat::source_location const& /*unused*/) {
   ++block_hits;
}

void
inner_handler(cat::source_location const& /*unused*/) {
   ++inner_hits;
}

void
thread_handler(cat::source_location const& /*unused*/) {
   ++thread_hits;
}

void
global_handler(cat::source_location const& /*unused*/) {
   ++global_hits;
}

void
explicit_handler(cat::source_location const& /*unused*/) {
   ++explicit_hits;
}

void
worker_fallback_handler(cat::source_location const& /*unused*/) {
   ++worker_fallback_hits;
}

}  // namespace

$test(assert_handler_block_scope) {
   cat::assert_handler const p_original = cat::current_assert_handler();
   block_hits = 0u;

   {
      cat::scoped_assert_handler const guard(&block_handler);
      cat::verify(cat::current_assert_handler() == &block_handler);
      cat::verify(false);
      cat::maybe<int4> held = 1;
      cat::verify(held.verify() == 1);
   }

   cat::verify(block_hits == 1u);
   cat::verify(cat::current_assert_handler() == p_original);
}

$test(assert_handler_nested_blocks) {
   inner_hits = 0u;
   block_hits = 0u;

   {
      cat::scoped_assert_handler const outer(&block_handler);
      cat::verify(false);
      {
         cat::scoped_assert_handler const inner(&inner_handler);
         cat::verify(false);
      }
      cat::verify(false);
   }

   cat::verify(inner_hits == 1u);
   cat::verify(block_hits == 2u);
}

$test(assert_handler_thread_local_scope) {
   cat::assert_handler const p_original = cat::current_assert_handler();
   thread_hits = 0u;
   worker_fallback_hits.store(0u);

   cat::set_thread_local_assert_handler(&thread_handler);
   cat::verify(cat::current_assert_handler() == &thread_handler);

   // A spawned thread has its own handler, and `test_fail` exits non-parent
   // threads, so the worker needs one of its own.
   cat::thread worker;
   worker
      .spawn(
         pager, 2_uki,
         [] {
            cat::set_thread_local_assert_handler(&worker_fallback_handler);
            cat::verify(false);
         }
      )
      .verify();
   worker.join().verify();
   worker.free(pager);

   cat::verify(false);
   cat::reset_thread_local_assert_handler();

   cat::verify(thread_hits == 1u);
   cat::verify(worker_fallback_hits.load() == 1u);
   cat::verify(cat::current_assert_handler() == p_original);
}

$test(assert_handler_global_scope_worker_inherits) {
   cat::assert_handler const p_original = cat::current_assert_handler();
   worker_fallback_hits.store(0u);

   cat::set_global_assert_handler(&worker_fallback_handler);

   cat::thread worker;
   worker
      .spawn(
         pager, 2_uki,
         [] {
            cat::verify(
               cat::current_assert_handler() == &worker_fallback_handler
            );
            cat::verify(false);
         }
      )
      .verify();
   worker.join().verify();
   worker.free(pager);

   cat::verify(cat::current_assert_handler() == &worker_fallback_handler);
   cat::verify(false);

   cat::set_global_assert_handler(p_original);

   cat::verify(worker_fallback_hits.load() == 2u);
   cat::verify(cat::current_assert_handler() == p_original);
}

$test(assert_handler_scope_precedence) {
   cat::assert_handler const p_original = cat::current_assert_handler();
   block_hits = 0u;
   thread_hits = 0u;
   global_hits = 0u;

   cat::set_global_assert_handler(&global_handler);
   cat::verify(false);

   cat::set_thread_local_assert_handler(&thread_handler);
   cat::verify(false);

   {
      cat::scoped_assert_handler const block_guard(&block_handler);
      cat::verify(false);
   }

   cat::verify(false);

   cat::reset_thread_local_assert_handler();
   cat::verify(false);

   cat::set_global_assert_handler(p_original);

   cat::verify(global_hits == 2u);
   cat::verify(thread_hits == 2u);
   cat::verify(block_hits == 1u);
}

$test(assert_handler_explicit_overrides_scope) {
   block_hits = 0u;
   explicit_hits = 0u;

   cat::scoped_assert_handler const guard(&block_handler);
   cat::verify(false, &explicit_handler);
   cat::verify(block_hits == 0u);
   cat::verify(explicit_hits == 1u);
}

$test(assert_handler_set_global) {
   cat::assert_handler const p_original = cat::current_assert_handler();
   global_hits = 0u;

   cat::set_global_assert_handler(&global_handler);
   cat::verify(false);
   cat::set_global_assert_handler(p_original);

   cat::verify(global_hits == 1u);
   cat::verify(cat::current_assert_handler() == p_original);
}
