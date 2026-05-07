#include <cat/linux>

// This is a minimal echo of equal size to 64-bit:
// https://blog.mandejan.nl/posts/smallest-echo.html
//
// With linker optimizations, we actually get 355 bytes.

auto
main(int argc, char* p_argv[]) -> int {
   if (argc <= 1) {
      auto _ = cat::println();
      return 0;
   }

   // Linux lays out argv strings contiguously so we replace null terminators
   // with spaces and the final null with a newline, then print the whole
   // string in a single string.
   // TODO: Replace this with a substring replace function.
   char* p_end = p_argv[1];
   for (cat::idx i = 1; i < argc; ++i) {
      while (*p_end != 0) {
         ++p_end;
         // Prevent a large `vpcmpistri` sequence that bloats `.text` by ~200 bytes.
         // Currently libCat struggles with the attributes on `main()` to fix this.
         asm volatile("" : "+r"(p_end));
      }
      *p_end++ = (i == argc - 1) ? '\n' : ' ';
   }

   cat::idx length{p_end - p_argv[1]};
   cat::str_view line = {p_argv[1], length};
   auto _ = cat::print(line);
}
