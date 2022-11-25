#include <cat/array>
#include <cat/math>
#include <cat/numerals>
#include <cat/page_allocator>
#include <cat/utility>

#include "../unit_tests.hpp"

int4 paging_counter_1 = 0;
int4 paging_counter_2 = 0;

struct TestType {
    TestType() {
        ++paging_counter_1;
    }
    ~TestType() {
        ++paging_counter_2;
    }
};

TEST(test_paging_memory) {
    // Initialize an allocator.
    cat::PageAllocator allocator;

    // Allocate and free a `const` page.
    cat::memoryHandle auto const const_memory =
        allocator.alloc<cat::Byte>().or_exit("Failed to page memory!");
    _ = allocator.get(const_memory);
    allocator.free(const_memory);

    // Allocate a page.
    cat::memoryHandle auto memory =
        allocator.alloc_multi<int4>(1'000).or_exit("Failed to page memory!");
    // Free the page at the end of this program.
    DEFER(allocator.free(memory);)

    // Write to the page.
    cat::Span page_span = allocator.get(memory);
    page_span[0] = 10;
    cat::verify(allocator.get(memory)[0] == 10);

    // Allocation with small-size optimization.
    int stack_variable;
    auto small_memory_1 = allocator.inline_alloc<int4>().or_exit();
    allocator.get(small_memory_1) = 2;
    // Both values should be on stack, so these addresses are close
    // together.
    cat::verify(small_memory_1.is_inline());
    // The handle's address should be the same as the data's if it was
    // allocated on the stack.
    int4& intref = *static_cast<int4*>(static_cast<void*>(&small_memory_1));
    intref = 10;
    cat::verify(allocator.get(small_memory_1) == 10);

    allocator.free(small_memory_1);

    auto small_memory_5 = allocator.inline_alloc_multi<int4>(1'000).or_exit();
    // `small_memory_1` should be in a page, so these addresses are far
    // apart.
    cat::verify(cat::abs(intptr<int>{&stack_variable} -
                         intptr<int4>{allocator.get(small_memory_5).data()}) >
                512);
    allocator.free(small_memory_1);

    // Small-size handles have unique storage.
    auto small_memory_2 = allocator.inline_alloc<int4>().or_exit();
    allocator.get(small_memory_2) = 1;
    auto small_memory_3 = allocator.inline_alloc<int4>().or_exit();
    allocator.get(small_memory_3) = 2;
    auto small_memory_4 = allocator.inline_alloc<int4>().or_exit();
    allocator.get(small_memory_4) = 3;
    cat::verify(allocator.get(small_memory_2) == 1);
    cat::verify(allocator.get(small_memory_3) == 2);
    cat::verify(allocator.get(small_memory_4) == 3);

    // Test constructor being called.
    cat::Maybe testtype = allocator.alloc<TestType>();
    allocator.free(testtype.value());

    // That constructor increments `paging_counter_1`.
    cat::verify(paging_counter_1 == 1);
    // That destructor increments `paging_counter_2`.
    cat::verify(paging_counter_2 == 1);

    // Test multi-allocations.
    auto array_memory = allocator.alloc_multi<TestType>(9).or_exit();
    // Those 9 constructors increment `paging_counter_2`.
    cat::verify(paging_counter_1 == 10);

    allocator.free(array_memory);
    // Those 9 destructors increment `paging_counter_2`.
    cat::verify(paging_counter_2 == 10);

    auto smalltesttype = allocator.inline_alloc<TestType>().or_exit();
    allocator.get(smalltesttype) = TestType{};
    allocator.free(smalltesttype);

    // Aligned memory allocations.
    auto aligned_mem = allocator.align_alloc_multi<int4>(32u, 4).or_exit();
    allocator.get(aligned_mem)[0] = 10;
    cat::verify(allocator.get(aligned_mem)[0] == 10);
    allocator.free(aligned_mem);
};
