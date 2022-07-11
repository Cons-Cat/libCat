#include <cat/allocators>
#include <cat/x11>

int main() {
    cat::PageAllocator allocator;
    // TODO: Work on this more.
    [[maybe_unused]] x11::Connection connection =
        x11::initiate_connection(allocator).or_exit(
            "Failed to create an X context!");
    cat::exit();
}
