#include <cat/allocator_page>
#include <cat/x11>



auto
main() -> int {
   cat::allocator_page allocator;
   // TODO: Work on this more.
   [[maybe_unused]]
   x11::connection x_connection =
      x11::initiate_connection(allocator).verify();
}
