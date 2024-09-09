#include "./unit_tests.hpp"

#include <cat/format>
#include <cat/page_allocator>

// The jump buffer must be constructed in `main()` instead of globally so that
// it can be guaranteed to occur before any unit tests are called.
namespace {
inline constinit cat::jmp_buffer* p_jump_buffer = nullptr;
}  // namespace

namespace cat {
void
test_fail(cat::source_location const& source_location) {
    cat::detail::print_assert_location(source_location);
    auto _ = cat::println();
    ++tests_failed;
    cat::longjmp(*p_jump_buffer, 2);
}
}  // namespace cat

using constructor_fn = void (*const)();
extern "C" {
extern constructor_fn __init_array_start[];  // NOLINT
extern constructor_fn __init_array_end[];    // NOLINT
}

[[gnu::optimize(0)]]
auto
main() -> int {
    // Change the default assert handler.
    cat::assert_handler = &cat::test_fail;

    // Set the jump buffer pointer before any constructors are called.
    cat::jmp_buffer jump_buffer;
    p_jump_buffer = &jump_buffer;

    // Load and call all functions with the attribute `[[gnu::constructor]]`.
    // The `TEST` macro declares these functions.

    // Call all constructor functions, including unit tests:
    for (constructor_fn const* pp_ctor_func = __init_array_start;
         pp_ctor_func < __init_array_end; ++pp_ctor_func) {
        last_ctor_was_test = false;

        constructor_fn p_ctor = *pp_ctor_func;
        // If this constructor is a unit test, it sets the previous flags.
        p_ctor();

        // TODO: The following code block is never reached except on the last
        // test with optimizations enabled.

        // Jump here when a test fails, skipping the rest of a test's
        // constructor function.
        cat::setjmp(jump_buffer);
    }

    // TODO: This will leak. An `inline_allocator` should be used.
    auto _ =
        cat::print(cat::fmt(pager, "\n{} tests passed.\n{} tests failed.\n",
                            tests_passed, tests_failed)
                       .or_exit());
}
