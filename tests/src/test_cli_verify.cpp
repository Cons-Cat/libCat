// Clang `-verify` checks of the `[[gsl::Pointer]]` and
// `[[clang::lifetimebound]]` annotations in `<cat/cli>`. Run with
// `just test safety`.

#include <cat/allocator_linear>
#include <cat/cli>
#include <cat/string>

using namespace cat::arithmetic;

// Positive cases: every function below must compile with ZERO warnings.

// A `cli` borrows `argv`, not the temporary `cli` that `.add_flag()` is called
// on.
auto
valid_cli_chain(
   int argc, char const* const* pp_argv [[clang::lifetimebound]]
) {
   auto cli = cat::cli(argc, pp_argv).add_flag<"n">().add_flag<"f,file">();
   return cli;
}

auto
valid_cli_parse(int argc, char const* const* pp_argv) -> cat::maybe<int4> {
   return cat::cli(argc, pp_argv).add_flag<"n">().parse<int4>("n");
}

// Negative cases: a `cli` outlives `argv`.

auto
dangling_cli_argv() {
   char const* argv[] = {"program", "-n", "1", nullptr};
   // expected-warning@+1 1+ {{local variable 'argv'}}
   return cat::cli(3, argv);
}

auto
dangling_cli_argv_chain() {
   char const* argv[] = {"program", "-n", "1", nullptr};
   // expected-warning@+1 1+ {{local variable 'argv'}}
   return cat::cli(3, argv).add_flag<"n">().add_flag<"m">();
}

auto
dangling_cli_argv_local() {
   char const* argv[] = {"program", "-n", "1", nullptr};
   // expected-warning@+1 1+ {{local variable 'argv'}}
   auto cli = cat::cli(3, argv).add_flag<"n">();
   return cli;
}

// Negative cases: a result from `.parse()` outlives its allocator.

auto
dangling_cli_parse_int_allocator_dyn(
   cat::span<cat::byte> arena, int argc, char const* const* pp_argv
) -> cat::maybe<int4> {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+3 1+ {{local variable 'linear'}}
   return cat::cli(argc, pp_argv)
      .add_flag<"n">()
      .parse<int4>(cat::allocator_dyn(linear), "n");
}

auto
dangling_cli_str_vec_allocator_dyn(
   cat::span<cat::byte> arena, int argc, char const* const* pp_argv
) -> cat::maybe<cat::str_vec> {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+3 1+ {{local variable 'linear'}}
   return cat::cli(argc, pp_argv)
      .add_flag<"f">()
      .parse<cat::str_vec>(cat::allocator_dyn(linear), "f");
}

auto
dangling_cli_raii_str_vec_allocator_dyn(
   cat::span<cat::byte> arena, int argc, char const* const* pp_argv
) {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+3 1+ {{local variable 'linear'}}
   return cat::cli(argc, pp_argv)
      .add_flag<"f">()
      .parse<cat::raii::str_vec<>>(cat::allocator_dyn(linear), "f");
}

auto
dangling_cli_raii_str_vec_allocator(
   cat::span<cat::byte> arena, int argc, char const* const* pp_argv
) {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+4 1+ {{local variable 'linear'}}
   return cat::cli(argc, pp_argv)
      .add_flag<"f">()
      .parse<cat::raii::str_vec<cat::allocator_linear>, cat::allocator_linear>(
         linear, "f"
      );
}
