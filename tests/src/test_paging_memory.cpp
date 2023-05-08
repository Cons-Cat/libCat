#include <cat/array>
#include <cat/math>
#include <cat/page_allocator>
#include <cat/utility>

#include "../unit_tests.hpp"

int4 paging_counter_1 = 0;
int4 paging_counter_2 = 0;

struct test_page_type {
    test_page_type() {
        ++paging_counter_1;
    }

    ~test_page_type() {
        ++paging_counter_2;
    }
};

TEST(test_paging_memory) {
    // Initialize an allocator.
    cat::page_allocator allocator;

    // Allocate a page.
    cat::span memory =
        allocator.alloc_multi<int4>(1'000u).or_exit("Failed to page memory!");
    // Free the page at the end of this program.
    defer(allocator.free(memory);)

    // Write to the page.
    memory[0] = 10;
    cat::verify(memory[0] == 10);

    // allocation_type with small-size optimization.
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

    auto small_memory_5 = allocator.inline_alloc_multi<int4>(1'000u).or_exit();
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
    cat::maybe testtype = allocator.alloc<test_page_type>();
    allocator.free(testtype.value());

    // That constructor increments `paging_counter_1`.
    cat::verify(paging_counter_1 == 1);
    // That destructor increments `paging_counter_2`.
    cat::verify(paging_counter_2 == 1);

    // Test multi-allocations.
    auto array_memory = allocator.alloc_multi<test_page_type>(9u).or_exit();
    // Those 9 constructors increment `paging_counter_2`.
    cat::verify(paging_counter_1 == 10);

    allocator.free(array_memory);
    // Those 9 destructors increment `paging_counter_2`.
    cat::verify(paging_counter_2 == 10);

    auto smalltesttype = allocator.inline_alloc<test_page_type>().or_exit();
    allocator.get(smalltesttype) = test_page_type();
    allocator.free(smalltesttype);

    // Aligned memory allocations.
    auto aligned_mem = allocator.align_alloc_multi<int4>(32u, 4u).or_exit();
    aligned_mem[0] = 10;
    cat::verify(aligned_mem[0] == 10);
    allocator.free(aligned_mem);
};
