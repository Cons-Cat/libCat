#include <cat/linux>

// TODO: Simplify with `cat::span`.
// TODO: Make this cross-platform.
auto
main(int argc, char* p_argv[]) -> int {
   for (cat::iword i = 1; i < argc; ++i) {
      cat::iword length = 0;
      // TODO: Use a `string_length_scalar()`.
      while (p_argv[i.raw][length.raw] != 0) {
         ++length;
      }

      p_argv[i.raw][length.raw] = ' ';
      auto _ = nix::sys_write(nix::stdout, p_argv[i.raw], length + 1);
   }

   auto _ = nix::sys_write(nix::stdout, "\n");
}
