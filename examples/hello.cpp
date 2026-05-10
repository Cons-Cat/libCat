#include <cat/simd_dispatch>
#include <cat/string>

auto
main() -> int {
   // In a robust `hello`, we would handle incomplete prints with `.or_exit()`.
   auto _ = cat::print("Meow, world!\n");
}
