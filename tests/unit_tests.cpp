#include "./unit_tests.hpp"

#include <cat/format>
#include <cat/page_allocator>

inline cat::JmpBuffer test_jump_buffer;

void test_assert_handler(cat::SourceLocation const& source_location) {
    cat::detail::print_assert_location(source_location);
    cat::longjmp(test_jump_buffer, 1);
}

using Constructor = void (*)();
extern "C" {
extern Constructor __init_array_start;  // NOLINT
extern Constructor __init_array_end;    // NOLINT
}

auto main() -> int {
    int4 tests_passed = 0;
    int4 tests_failed = 0;

    // Load and call all functions with the attribute
    // `[[gnu::constructor]]`. The `TEST` macro declares these functions.
    Constructor* p_func = &__init_array_start;

    // Run all unit tests:
    for (; p_func < &__init_array_end; ++p_func) {
        if (cat::setjmp(test_jump_buffer) == 0) {
            (*p_func)();
            // If no assert failed, run next test:
            ++tests_passed;
            continue;
        }

        // If a `cat::longjmp()` call came from `test_assert_handler()`:
        ++tests_failed;
        _ = cat::print("\n");
        _ = cat::eprintln("Test failed!");
    }

    cat::PageAllocator allocator;
    _ = cat::print(cat::format(allocator,
                               "{} tests passed.\n{} tests failed.\n",
                               // Subtract one, because there is one constructor
                               // call that isn't a unit test.
                               tests_passed - 1, tests_failed)
                       .value());
}
