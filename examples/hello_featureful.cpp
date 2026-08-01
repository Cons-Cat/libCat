#include <cat/debug>
#include <cat/format>
#include <cat/page_allocator>
#include <cat/string>

auto
main() -> int {
   auto pager = cat::page_allocator();
   auto _ = cat::print_fmt(pager, "Meow, world!\n").verify();
}
