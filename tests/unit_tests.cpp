#include "./unit_tests.hpp"

#include <cat/format>
#include <cat/page_allocator>

// The jump buffer must be constructed in `main()` instead of globally so that
// it can be guaranteed to occur before any unit tests are called.
namespace {
inline constinit cat::jmp_buffer* p_jump_buffer = nullptr;
}  // namespace

namespace cat {
void
test_fail(cat::source_location const& source_location) {
   cat::detail::print_assert_location(source_location);
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
   // Change the default assert handler.
   cat::assert_handler = &cat::test_fail;

   // Set the jump buffer pointer before any constructors are called.
   cat::jmp_buffer jump_buffer;
   p_jump_buffer = &jump_buffer;

   // Call all unit test functions that were pushed into `test_fns` by the
   // `CAT_TEST` macro.
   for (cat::idx i = 0; i < test_fns.size(); ++i) {
      auto _ = ::cat::print(::cat::fmt(pager, "Running test {}", i).value());
      if (cat::setjmp(jump_buffer)) {
         // Jump here when a test fails, skipping the rest of a test's
         // constructor function.
         continue;
      }
      (reinterpret_cast<constructor_fn>(test_fns[i]))();
   }

   // `tests_passed` and `tests_failed` are modified within the `CAT_TEST` macro.
   // TODO: This will leak. An `inline_allocator` should be used.
   auto _ = cat::print(cat::fmt(pager, "\n{} tests passed.\n{} tests failed.\n",
                                tests_passed, tests_failed)
                          .or_exit());
}
