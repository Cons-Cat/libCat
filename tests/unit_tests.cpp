#include "./unit_tests.hpp"

#include <cat/allocator_inplace>
#include <cat/atomic>
#include <cat/cli>
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

struct registered_test {
   cat::str_view name;
   void* p_fn;
};

constinit cat::vec_big<registered_test> test_fns;

// Match `text` against a glob `pattern`.
// TODO: `cat::cli` should provide a high quality glob feature.
constexpr auto
glob_match(cat::str_view pattern, cat::str_view text) -> bool {
   idx pattern_index = 0u;
   idx text_index = 0u;
   // Where to resume after the last `*` when a later character mismatches.
   idx star_index = pattern.size();
   idx star_text_index = 0u;

   while (text_index < text.size()) {
      if (
         pattern_index < pattern.size()
         && pattern[pattern_index] == text[text_index]
      ) {
         ++pattern_index;
         ++text_index;
      } else if (
         pattern_index < pattern.size() && pattern[pattern_index] == '*'
      ) {
         star_index = pattern_index;
         star_text_index = text_index;
         ++pattern_index;
      } else if (star_index != pattern.size()) {
         pattern_index = star_index + 1u;
         text_index = ++star_text_index;
      } else {
         return false;
      }
   }

   while (pattern_index < pattern.size() && pattern[pattern_index] == '*') {
      ++pattern_index;
   }
   return pattern_index == pattern.size();
}

// Test this here for now. It'll move when glob moves.
static_assert(glob_match("mutex_*", "mutex_lock_unlock"));
static_assert(glob_match("*lock*", "mutex_lock_unlock"));
static_assert(glob_match("*", ""));
static_assert(glob_match("a*b*c", "aXbYbZc"));
static_assert(!glob_match("mutex_*", "tsan_mutex_unlocked_access"));
static_assert(!glob_match("abc", "abcd"));

// If `name` is selected by `patterns`, which are globs if `is_glob`, otherwise
// exact names. No patterns select every test.
auto
is_selected(
   cat::span<char const* _Nonnull const> patterns, bool is_glob,
   cat::str_view name
) -> bool {
   if (patterns.is_empty()) {
      return true;
   }

   for (char const* _Nonnull const p_pattern : patterns) {
      cat::str_view const pattern = cat::zstr_view(p_pattern);
      if (is_glob ? glob_match(pattern, name) : pattern == name) {
         return true;
      }
   }

   return false;
}

}  // namespace

void
register_test(cat::str_view name, constructor_fn p_test_fn) {
   if (test_fns.capacity() == 0u) {
      test_fns.reserve(g_tests_alloc, 2_uki).value();
   }
   auto _ = test_fns.push_back(
      g_tests_alloc, {name, reinterpret_cast<void*>(p_test_fn)}
   );
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
extern "C" {
auto
__tsan_default_options() -> char const* _Nullable {
   // If we skip reporting some races in the tests, we get false negatives.
   return "suppress_equal_stacks=0:suppress_equal_addresses=0";
}
}

// Count and hide each report that `expect_tsan_report()` expects. Unexpected
// reports are printed, and TSan fails the run for them.
auto
__tsan::OnReport(ReportDesc const* _Nonnull p_report, bool is_suppressed)
   -> bool {
   if (is_suppressed || !detail::is_expecting_tsan_reports.load()) {
      return is_suppressed;
   }

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
      const_cast<__tsan::ReportDesc*>(p_report), &p_description, &count,
      &stack_count, &memory_operation_count, &location_count, &mutex_count,
      &thread_count, &unique_thread_id_count, &p_sleep_trace, 0u
   );

   cat::str_view const description =
      p_description != nullptr ? cat::str_view(p_description) : "";
   if (description == "data-race") {
      ++detail::tsan_data_races;
   } else if (description == "lock-order-inversion") {
      ++detail::tsan_lock_order_inversions;
   } else {
      ++detail::tsan_other_reports;
   }
   return true;
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

   // Run only the tests named in the command, as e.g.
   // `just test debug --glob 'mutex_*' thread`

   // All tests run by default if you don't specify.
   auto arguments = cat::detail::get_command_arguments();
   char const* const no_arguments[] = {nullptr};
   auto cli = arguments.is_empty()
                 ? cat::cli(0, no_arguments)
                 : cat::cli(int(arguments.size()), arguments.data());

   auto const selecting = cli.add_bool<"g,glob">();
   bool const is_glob = selecting.parse<bool>("glob").value_or(false);
   auto const patterns = selecting.positionals();

   for (char const* _Nonnull const p_pattern : patterns) {
      bool is_found = false;
      for (registered_test const& test : test_fns) {
         if (is_selected({&p_pattern, 1u}, is_glob, test.name)) {
            is_found = true;
            break;
         }
      }

      // Detect an erroneous empty glob list.
      if (!is_found) {
         print_allocator.reset();
         auto _ = cat::eprint_fmt(
            print_allocator, "No test matches `{}`.\n",
            cat::zstr_view(p_pattern)
         );

         test_fns.free(g_tests_alloc);
         return 1;
      }
   }

   // Call all unit test functions that were pushed into `test_fns` by the
   // `$test` macro.
   for (idx i = 0; i < test_fns.size(); ++i) {
      if (!is_selected(patterns, is_glob, test_fns[i].name)) {
         continue;
      }

      print_allocator.reset();
      auto _ = cat::print_fmt(print_allocator, "Running test {}", i);
      if (cat::setjmp(jump_buffer)) {
         // Jump here when a test fails, skipping the rest of a test's
         // constructor function.
         cat::set_global_assert_handler(&cat::test_fail);
         cat::detail::reset_local_assert_handlers();
         continue;
      }
      (reinterpret_cast<constructor_fn>(test_fns[i].p_fn))();
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
