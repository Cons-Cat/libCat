#include <cat/debug>
#include <cat/format>
#include <cat/page_allocator>
#include <cat/vec>

// All unit tests have access to these symbols:
using namespace cat::literals;
using namespace cat::integers;

[[gnu::used]]
constinit inline cat::page_allocator pager;

constinit inline idx tests_passed;
constinit inline idx tests_failed;

using constructor_fn = void (*const)();
constinit inline cat::maybe<cat::vec<void*, cat::page_allocator>> test_fns;

[[noreturn]]
void
test_fail(cat::source_location const& source_location);

// This macro declares a unit test named `test_name`, which is executed
// automatically in this program's constructor calls.
#define CAT_TEST(test_name)                                                    \
   void test_##test_name();                                                    \
   void test_##test_name##_prologue();                                         \
                                                                               \
   [[gnu::constructor]]                                                        \
   void cat_register_test##test_name() {                                       \
      /* For some reason, linking asan causes this vector to be uninitialized. \
       * I've tried `[[gnu::constructor]]` priorities to order it, but that    \
       * doesn't work. This hacky check works around it.                       \
       */                                                                      \
      if (!test_fns.has_value()) {                                             \
         test_fns = cat::make_vec_reserved<void*>(pager, 4_uki / 8).value();   \
      }                                                                        \
      /* This is memory is pre-reserved, so `push_back` cannot fail. */        \
      auto _ = test_fns.verify().push_back(                                    \
         reinterpret_cast<void*>(test_##test_name##_prologue));                \
   }                                                                           \
                                                                               \
   void test_##test_name##_prologue() {                                        \
      /* TODO: Align the whitespace after `:` for 1 and 2 digit tests. */      \
      constexpr ::cat::str_view string = ": test_" #test_name "...\n";         \
      /* Gracefully handle print failure in unit tests.*/                      \
      auto _ = ::cat::print(string);                                           \
      test_##test_name();                                                      \
      ++tests_passed;                                                          \
   }                                                                           \
   void test_##test_name()

// `CAT_TEST` should never be `#undef`'d. The redefinable macro `test` exists
// to make this macro more ergonomic.
#pragma clang final(CAT_TEST)

#define test CAT_TEST
