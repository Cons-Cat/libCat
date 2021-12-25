
#include <page_allocator.h>

void meow() {
    debugger_entry_point();
    // Initialize an allocator.
    PageAllocator allocator;
    // Allocate a page.
    allocator.malloc(4).or_panic();
    // Write to the page.
    // allocator[0] = 10;
    // Result(allocator[0] == 10).or_panic();
    exit(0);
};
