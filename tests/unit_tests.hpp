#include <cat/debug>
#include <cat/linear_allocator>
#include <cat/page_allocator>
#include <cat/vector>

void test_assert_handler(cat::SourceLocation const& source_location);
extern cat::PageAllocator tests_page_allocator;
extern cat::LinearAllocator tests_linear_allocator;

// using TestFunction = void();
using TestFunction = int;

extern cat::Vector<TestFunction> all_tests;
