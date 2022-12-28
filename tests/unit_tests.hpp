#include <cat/debug>
#include <cat/format>
#include <cat/page_allocator>

constinit inline int8 tests_run = 0;
constinit inline bool last_ctor_was_test = false;
constinit inline cat::page_allocator pager;

void test_fail(cat::source_location const& source_location);

// The `cat::detail` namespace guarantees a unique name here.
namespace cat::detail {

// This macro declares a unit test named `test_name`, which is executed
// automatically in this program's constructor calls.
#define TEST(test_name)                                                     \
    void test_name();                                                       \
    [[gnu::constructor]] void _register_##test_name() {                     \
        using cat::fixed_string;                                            \
        _ = cat::print("Running test ");                                    \
        last_ctor_was_test = true;                                          \
        ++tests_run;                                                        \
        /* TODO: This will leak. An `Inlineallocator` should be used. */    \
        _ = cat::print(cat::format(pager, "{}", tests_run).value());        \
        /* TODO: Align the whitespace after `:` for 1 and 2 digit tests. */ \
        auto string = cat::fixed_string(": ") + #test_name + "...\n";       \
        _ = cat::print(string);                                             \
        test_name();                                                        \
    }                                                                       \
    void test_name()

}  // namespace cat::detail
