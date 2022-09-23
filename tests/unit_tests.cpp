#include "./unit_tests.hpp"

void test_assert_handler(cat::SourceLocation const& source_location) {
    cat::detail::print_assert_location(source_location);
}

using Constructor = void (*)();
extern "C" {
extern Constructor __init_array_start;  // NOLINT
extern Constructor __init_array_end;    // NOLINT
}

auto main() -> int {
    // Load and call all functions with the attribute `[[gnu::constructor]]`.
    // The `TEST` macro declares these functions.
    Constructor* p_func = &__init_array_start;
    for (; p_func < &__init_array_end; ++p_func) {
        (*p_func)();
    }
};
