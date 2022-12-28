#include <cat/page_allocator>
#include <cat/x11>

auto main() -> int {
    cat::page_allocator allocator;
    // TODO: Work on this more.
    [[maybe_unused]] x11::connection x_connection =
        x11::initiate_connection(allocator).or_exit(
            "Failed to create an X context!");
}
