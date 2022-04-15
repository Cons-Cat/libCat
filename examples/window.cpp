#include <allocators>
#include <x>

void meow() {
    cat::PageAllocator allocator;
    xwin::initiate_connection(allocator).or_panic(
        "Failed to create an X context!");
    cat::exit();
}
