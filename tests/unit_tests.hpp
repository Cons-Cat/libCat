#include <cat/debug>
#include <cat/format>
#include <cat/page_allocator>
#include <cat/vec>

// All unit tests have access to these symbols:
using namespace cat::literals;
using namespace cat::integers;

constinit inline cat::page_allocator pager;

constinit inline idx tests_passed;
constinit inline idx tests_failed;

using constructor_fn = void (*const)();
inline cat::vec test_fns =
   cat::make_vec_reserved<void*>(pager, 4_uki / 8).value();

[[noreturn]]
void
test_fail(cat::source_location const& source_location);

// This macro declares a unit test named `test_name`, which is executed
// automatically in this program's constructor calls.
#define TEST(test_name)                                                     \
   void test_name();                                                        \
   void test_name##_prologue();                                             \
                                                                            \
   [[gnu::constructor]]                                                     \
   void cat_register_##test_name() {                                        \
      auto _ =                                                              \
         test_fns.push_back(reinterpret_cast<void*>(test_name##_prologue)); \
   }                                                                        \
                                                                            \
   void test_name##_prologue() {                                            \
      /* TODO: Align the whitespace after `:` for 1 and 2 digit tests. */   \
      constexpr char string[] = ": " #test_name "...\n";                    \
      auto _ = ::cat::print(string);                                        \
      test_name();                                                          \
      ++tests_passed;                                                       \
   }                                                                        \
   void test_name()
