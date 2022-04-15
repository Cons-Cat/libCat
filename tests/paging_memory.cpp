#include <allocators>
#include <array>

void meow() {
    // Initialize an allocator.
    PageAllocator allocator;
    // Allocate a page.
    auto memory = allocator.malloc<int4>().or_panic();
    // Write to the page.
    int4& foo = memory.get();
    foo = 10;
    Result(*static_cast<int4*>(memory.as_address()) == 10).assert();
    // Free the page.
    allocator.free(memory).or_panic();

    cat::exit();
};
