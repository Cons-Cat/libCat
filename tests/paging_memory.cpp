#include <allocators>
#include <array>

void meow() {
    // Initialize an allocator.
    PageAllocator allocator;
    // Allocate a page.
    auto memory = allocator.malloc(32).or_panic();
    // Write to the page.
    auto& p_int = *static_cast<int4*>(memory.as_address());
    p_int = 10;
    Result(p_int == 10).or_panic();
    // Free the page.
    allocator.free(memory).or_panic();

    cat::exit();
};
