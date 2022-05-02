#include <allocators>
#include <x>

void meow() {
    cat::PageAllocator allocator;
    // TODO: Work on this more.
    [[maybe_unused]] xwin::Connection connection =
        xwin::initiate_connection(allocator).or_panic(
            "Failed to create an X context!");
    cat::exit();
}
