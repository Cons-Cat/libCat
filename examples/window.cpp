#include <cat/page_allocator>
#include <cat/x11>

auto main() -> int {
    cat::PageAllocator allocator;
    // TODO: Work on this more.
    [[maybe_unused]] x11::Connection connection =
        x11::initiate_connection(allocator).or_exit(
            "Failed to create an X context!");
}
