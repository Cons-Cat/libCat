#include <cat/debug>
#include <cat/format>
#include <cat/page_allocator>

// All unit tests have access to these symbols:
using namespace cat::literals;
using namespace cat::integers;

constinit inline idx tests_run;
constinit inline bool last_ctor_was_test = false;
constinit inline cat::page_allocator pager;

constinit inline idx tests_passed;
constinit inline idx tests_failed;

[[noreturn]]
void
test_fail(cat::source_location const& source_location);

// This macro declares a unit test named `test_name`, which is executed
// automatically in this program's constructor calls.
#define TEST(test_name)                                                   \
   void test_name();                                                      \
   /* For some reason, optimizations miscompile here. */                  \
   [[gnu::constructor, gnu::optimize(0), clang::optnone]]                 \
   void cat_register_##test_name() {                                      \
      auto _ = ::cat::print("Running test ");                             \
      last_ctor_was_test = true;                                          \
      ++tests_run;                                                        \
      /* TODO: This will leak. An `inline_allocator` should be used. */   \
      auto _ = ::cat::print(::cat::fmt(pager, "{}", tests_run).value());  \
      /* TODO: Align the whitespace after `:` for 1 and 2 digit tests. */ \
      constexpr char string[] = ": " #test_name "...\n";                  \
      auto _ = ::cat::print(string);                                      \
      test_name();                                                        \
      ++tests_passed;                                                     \
   }                                                                      \
   void test_name()
