#include <cstdio>
#include <cstdlib>

// `printf()` is actually optimized here better by GCC than `write()`,
// because its `strlen` invocation can be constant-folded.

auto
main() -> int32_t {
   printf("Hello, Conscat!\n");
}
