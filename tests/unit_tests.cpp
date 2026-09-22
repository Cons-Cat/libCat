#include "./unit_tests.hpp"

#include <cat/allocator_inplace>
#include <cat/atomic>
#include <cat/format>
#include <cat/thread>
#include <cat/vec>

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

auto
main() -> int {
   parent_thread_id = cat::this_thread::get_id();
   auto print_allocator = cat::make_allocator_inplace<128u>();

   // Change the default assert handler.
   cat::set_global_assert_handler(&cat::test_fail);

   // Set the jump buffer pointer before any constructors are called.
   cat::jmp_buffer jump_buffer;
   p_jump_buffer = &jump_buffer;

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
   cat::idx const n_passed = tests_passed.relaxed();
   cat::idx const n_failed = tests_failed.relaxed();

   print_allocator.reset();
   auto _ = cat::print_fmt(
      print_allocator, "\n{} tests passed.\n{} tests failed.\n", n_passed,
      n_failed
   );
   test_fns.free(g_tests_alloc);

   return static_cast<int>(n_failed != 0u);  // Return 0 or 1.
}
