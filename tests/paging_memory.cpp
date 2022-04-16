#include <allocators>
#include <array>
#include <math>

void meow() {
    // Initialize an allocator.
    cat::PageAllocator allocator;
    // Allocate a page.
    Optional maybe_memory = allocator.malloc<int4>(1_ki);
    if (!maybe_memory.has_value()) {
        cat::print_line("Failed to allocate memory!").discard_result();
        cat::exit(1);
    }
    auto memory = maybe_memory.value();

    // Write to the page.
    allocator.get(memory) = 10;
    Result(allocator.get(memory) == 10).or_panic();
    // Free the page.
    allocator.free(memory).or_panic();

    // Allocation with small-size optimization.
    int stack_variable;
    auto small_memory = allocator.malloca<int4>().value();
    allocator.get(small_memory) = 2;
    // Both values should be on stack, so these addresses are close
    // together.
    Result(cat::abs(intptr{&stack_variable} -
                    intptr{&allocator.get(small_memory)}) < 512)
        .or_panic();
    allocator.freea(small_memory).discard_result();

    small_memory = allocator.malloca<int4>(1_ki).value();
    // `small_memory` should be in a page, so these addresses are far
    // apart.
    Result(cat::abs(intptr{&stack_variable} -
                    intptr{&allocator.get(small_memory)}) > 512)
        .or_panic();
    allocator.freea(small_memory).discard_result();

    cat::exit();
};
