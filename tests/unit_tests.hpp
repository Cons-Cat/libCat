#include <cat/debug>
#include <cat/linear_allocator>
#include <cat/page_allocator>
#include <cat/vector>

void test_fail(cat::SourceLocation const& source_location);

// This macro declares a unit test named `test_name`, which is executed
// automatically in this program's constructor calls.
#define TEST(test_name)                                            \
    void test_name();                                              \
    [[gnu::constructor]] void register_##test_name() {             \
        using cat::StaticString;                                   \
        StaticString string =                                      \
            "Running test: " + StaticString{#test_name} + "...\n"; \
        _ = cat::print(string);                                    \
        test_name();                                               \
    }                                                              \
    void test_name()
