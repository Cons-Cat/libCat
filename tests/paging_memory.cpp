#include <allocators>
#include <array>

void meow() {
    // Initialize an allocator.
    cat::PageAllocator allocator;
    // Allocate a page.
    bool1 failed = false;
    auto memory = allocator.malloc<int4>()
                      .or_else([&]() {
                          failed = true;
                      })
                      .value();
    if (failed) {
        cat::exit(1);
    }

    // Write to the page.
    allocator.get(memory) = 10;
    Result(allocator.get(memory) == 10).assert();
    // Free the page.
    allocator.free(memory).or_panic();

    cat::exit();
};
