#include "./unit_tests.hpp"

#include <cat/allocator_inplace>
#include <cat/atomic>
#include <cat/exception>
#include <cat/format>
#include <cat/thread>
#include <cat/vec>

#include "./tsan_tests.hpp"

// The jump buffer must be constructed in `main()` instead of globally so that
// it can be guaranteed to occur before any unit tests are called.
namespace {
inline constinit cat::jmp_buffer* p_jump_buffer = nullptr;
constinit cat::thread::id parent_thread_id;

constinit cat::atomic<cat::idx> tests_passed{};
constinit cat::atomic<cat::idx> tests_failed{};
constinit cat::vec_big<void*> test_fns;
}  // namespace

void
register_test(constructor_fn p_test_fn) {
   if (test_fns.capacity() == 0u) {
      test_fns.reserve(g_tests_alloc, 2_uki).value();
   }
   auto _ =
      test_fns.push_back(g_tests_alloc, reinterpret_cast<void*>(p_test_fn));
}

void
run_test_prologue(cat::str_view label, constructor_fn p_test_fn) {
   auto _ = cat::print(label);
   p_test_fn();
   ++tests_passed;
}

namespace cat {
void
test_fail(cat::source_location const& source_location) {
   if (this_thread::get_id() != parent_thread_id) {
      nix::sys_exit(1);
   }
   cat::detail::print_assert_location(source_location);
   // Gracefully handle print in tests.
   auto _ = cat::println();
   ++tests_failed;
   cat::longjmp(*p_jump_buffer, 2);
}
}  // namespace cat

extern "C" {
extern constructor_fn __init_array_start[];
extern constructor_fn __init_array_end[];
}

#if __has_feature(thread_sanitizer)
namespace {
constinit cat::atomic<idx> unexpected_tsan_reports;
}

extern "C" {
auto
__tsan_default_options() -> char const* _Nullable {
   // If we skip reporting some races in the tests, we get false negatives.
   return "suppress_equal_stacks=0:suppress_equal_addresses=0";
}

// Count each report for `expect_tsan_report()`.
void
__tsan_on_report(void* _Nonnull p_report) {
   char const* p_description = nullptr;
   int4 count;
   int4 stack_count;
   int4 memory_operation_count;
   int4 location_count;
   int4 mutex_count;
   int4 thread_count;
   int4 unique_thread_id_count;
   void* p_sleep_trace = nullptr;
   auto _ = __tsan_get_report_data(
      p_report, &p_description, &count, &stack_count, &memory_operation_count,
      &location_count, &mutex_count, &thread_count, &unique_thread_id_count,
      &p_sleep_trace, 0u
   );

   if (!detail::is_expecting_tsan_reports.load()) {
      ++unexpected_tsan_reports;
      return;
   }

   cat::str_view const description =
      p_description != nullptr ? cat::str_view(p_description) : "";
   if (description == "data-race") {
      ++detail::tsan_data_races;
   } else if (description == "lock-order-inversion") {
      ++detail::tsan_lock_order_inversions;
   } else {
      ++detail::tsan_other_reports;
   }
}

// Expected reports must not fail the run, but unexpected reports still do.
auto
__tsan_on_finalize(int4 /*unused*/) -> int4 {
   return unexpected_tsan_reports.load() == 0u ? 0 : 1;
}
}
#endif

auto
main() -> int {
   parent_thread_id = cat::this_thread::get_id();
   auto print_allocator = cat::make_allocator_inplace<128u>();

   // Change the default assert handler.
   cat::set_global_assert_handler(&cat::test_fail);

   // Set the jump buffer pointer before any constructors are called.
   cat::jmp_buffer jump_buffer;
   // `jump_buffer` outlives every use of `p_jump_buffer`.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wlifetime-safety-dangling-global-in-main"
   p_jump_buffer = &jump_buffer;
#pragma clang diagnostic pop

   // Register the exception allocator once before any test runs.
   cat::alloc_exception_runtime(g_tests_alloc);

   // Call all unit test functions that were pushed into `test_fns` by the
   // `$test` macro.
   for (idx i = 0; i < test_fns.size(); ++i) {
      print_allocator.reset();
      auto _ = cat::print_fmt(print_allocator, "Running test {}", i);
      if (cat::setjmp(jump_buffer)) {
         // Jump here when a test fails, skipping the rest of a test's
         // constructor function.
         cat::set_global_assert_handler(&cat::test_fail);
         cat::detail::reset_local_assert_handlers();
         continue;
      }
      (reinterpret_cast<constructor_fn>(test_fns[i]))();
   }

   // `tests_passed` and `tests_failed` are modified within the `$test`
   // macro.
   cat::idx const n_passed = tests_passed.order_relaxed();
   cat::idx const n_failed = tests_failed.order_relaxed();

   print_allocator.reset();
   auto _ = cat::print_fmt(
      print_allocator, "\n{} tests passed.\n{} tests failed.\n", n_passed,
      n_failed
   );
   test_fns.free(g_tests_alloc);

   return static_cast<int>(n_failed != 0u);  // Return 0 or 1.
}
