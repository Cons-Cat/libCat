#include <cat/debug>
#include <cat/format>
#include <cat/page_allocator>

// All unit tests have access to these symbols:
using namespace cat::literals;
using namespace cat::integers;

constinit inline int8 tests_run = 0;
constinit inline bool last_ctor_was_test = false;
constinit inline cat::page_allocator pager;

void test_fail(cat::source_location const& source_location);

// This macro declares a unit test named `test_name`, which is executed
// automatically in this program's constructor calls.
#define TEST(test_name)                                                     \
    void test_name();                                                       \
    [[gnu::constructor]]                                                    \
    void cat_register_##test_name() {                                       \
        auto _ = cat::print("Running test ");                               \
        last_ctor_was_test = true;                                          \
        ++tests_run;                                                        \
        /* TODO: This will leak. An `inline_allocator` should be used. */   \
        auto _ = cat::print(cat::format(pager, "{}", tests_run).value());   \
        /* TODO: Align the whitespace after `:` for 1 and 2 digit tests. */ \
        constexpr auto string = ": " #test_name "...\n";                    \
        auto _ = cat::print(string);                                        \
        test_name();                                                        \
    }                                                                       \
    void test_name()
