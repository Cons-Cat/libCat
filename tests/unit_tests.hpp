#include <cat/debug>
#include <cat/string>

constinit inline int4 total_tests = 0;

void test_fail(cat::SourceLocation const& source_location);

// This macro declares a unit test named `test_name`, which is executed
// automatically in this program's constructor calls.
#define TEST(test_name)                                            \
    void test_name();                                              \
    [[gnu::constructor]] void register_##test_name() {             \
        using cat::StaticString;                                   \
        ++total_tests;                                             \
        StaticString string =                                      \
            "Running test: " + StaticString{#test_name} + "...\n"; \
        _ = cat::print(string);                                    \
        test_name();                                               \
    }                                                              \
    void test_name()
