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

struct TestClass {
    TestClass() {
        _ = cat::print("MOO!\n");
    }
};

TestClass testerT;

extern "C" {
extern void (*__CTOR_LIST__)();  // NOLINT
extern void (*__DTOR_LIST__)();  // NOLINT
}

void call_constructors() {
    // Load the list of constructors.
    void (**p_constructor)() = &__CTOR_LIST__;

    // The first value in this list is the number of constructors.
    int count_constructors = *reinterpret_cast<int*>(p_constructor);

    // Advance the iterator pointer.
    ++p_constructor;

    // Loop through the constructors to call.
    while (count_constructors != 0) {
        _ = cat::println("Foo");
        (*p_constructor)();
        --count_constructors;
        ++p_constructor;
    }
}

auto main() -> int {
    call_constructors();
};
