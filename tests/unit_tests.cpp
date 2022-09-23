#include "./unit_tests.hpp"

void test_assert_handler(cat::SourceLocation const& source_location) {
    cat::detail::print_assert_location(source_location);
}

// Allocate memory to store handles to all unit tests.
cat::PageAllocator tests_page_allocator;
cat::LinearAllocator tests_linear_allocator =
    cat::LinearAllocator::backed(tests_page_allocator, 2_ki)
        .or_exit("Failed to allocate any memory!");

// Function pointers to unit tests are appended to `all_tests`.
cat::Vector<TestFunction> all_tests =
    cat::Vector<TestFunction>::reserved(tests_linear_allocator, 100).or_exit();

using Constructor = void (*)();
extern "C" {
extern Constructor __init_array_start;  // NOLINT
extern Constructor __init_array_end;    // NOLINT
}

auto main() -> int {
    Constructor* p_func = &__init_array_start;
    for (; p_func < &__init_array_end; ++p_func) {
        (*p_func)();
    }
    // static_init();
};
