#include <cat/debug>
#include <cat/format>
#include <cat/allocator_page>
#include <cat/string>

auto
main() -> int {
   auto pager = cat::allocator_page();
   auto _ = cat::print_fmt(pager, "Meow, world!\n").verify();
}
