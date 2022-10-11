#include "./unit_tests.hpp"

#include <cat/format>
#include <cat/page_allocator>

constinit cat::JmpBuffer* p_test_jump_buffer;

void test_fail(cat::SourceLocation const& source_location) {
    cat::detail::print_assert_location(source_location);
    cat::longjmp(*p_test_jump_buffer, 1);
}

using Constructor = void (*)();
extern "C" {
extern Constructor __init_array_start;  // NOLINT
extern Constructor __init_array_end;    // NOLINT
}

auto main() -> int {
    // Change the default assert handler.
    cat::assert_handler = test_fail;

    cat::JmpBuffer jump_buffer;
    p_test_jump_buffer = &jump_buffer;
    int8 tests_passed = 0;
    int8 tests_failed = 0;

    // Load and call all functions with the attribute
    // `[[gnu::constructor]]`. The `TEST` macro declares these functions.
    Constructor* p_func = &__init_array_start;

    int4 temp_tests = total_tests;

    // Run all unit tests:
    for (; p_func < &__init_array_end; ++p_func) {
        if (cat::setjmp(jump_buffer) == 0) {
            (*p_func)();
            // If no assert failed, run next test:
            if (temp_tests != total_tests) {
                ++tests_passed;
                temp_tests = total_tests;
            }
            continue;
        }

        // If a `cat::longjmp()` call came from `test_fail()`:
        ++tests_failed;
        _ = cat::print("\n");
    }

    cat::PageAllocator allocator;
    _ = cat::print(cat::format(allocator,
                               "{} tests passed.\n{} tests failed.\n",
                               // Subtract one, because there is one constructor
                               // call that isn't a unit test.
                               tests_passed, tests_failed)
                       .value());
}
