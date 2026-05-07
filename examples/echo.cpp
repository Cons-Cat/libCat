#include <cat/linux>

auto
main(int argc, char* p_argv[]) -> int {
   // Skip the 0th argument, the program name.
   for (cat::idx i = 1; i < argc; ++i) {
      cat::idx length = 0;
      // TODO: Use a `string_length_scalar()`.
      while (p_argv[i][length] != 0) {
         ++length;
      }

      // Replace the null terminator with a space.
      p_argv[i][length] = ' ';
      ++length; // Include that space in print output.
      cat::str_view string = {p_argv[i], length };
      auto _ = cat::print(string);
   }

   auto _ = cat::println();
}
