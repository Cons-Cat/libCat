#include "./unit_tests.hpp"

#include <cat/format>
#include <cat/page_allocator>

// The jump buffer must be constructed in `main()` instead of globally so that
// it can be guaranteed to occur before any unit tests are called.
constinit cat::JmpBuffer* p_jump_buffer = nullptr;

void test_fail(cat::SourceLocation const& source_location) {
    cat::detail::print_assert_location(source_location);
    _ = cat::println();
    cat::longjmp(*p_jump_buffer, 2);
    __builtin_unreachable();
}

using Constructor = void (*)();
extern "C" {
extern Constructor __init_array_start;  // NOLINT
extern Constructor __init_array_end;    // NOLINT
}

auto main() -> int {
    // Change the default assert handler.
    cat::assert_handler = &test_fail;

    // Set the jump buffer pointer before any constructors are called.
    cat::JmpBuffer jump_buffer;
    p_jump_buffer = &jump_buffer;

    int tests_passed = 0;
    int8 tests_failed = 0;

    // Load and call all functions with the attribute
    // `[[gnu::constructor]]`. The `TEST` macro declares these functions.
    Constructor* p_func = &__init_array_start;

    // Call all constructor functions, including unit tests:
    for (; p_func < &__init_array_end; ++p_func) {
        last_ctor_was_test = false;

        // If this constructor is a unit test, it sets the previous flags.
        (*p_func)();

        // Increment the success/fail counters.
        if (last_ctor_was_test) {
            if (cat::setjmp(jump_buffer) == 0) {
                ++tests_passed;
            } else {
                ++tests_failed;
            }
        }
    }

    // TODO: This always prints 1 test passed with optimizations.
    // TODO: This will leak. An `InlineAllocator` should be used.
    _ = cat::print(cat::format(pager, "\n{} tests passed.\n{} tests failed.\n",
                               tests_passed, tests_failed)
                       .or_exit());
}
