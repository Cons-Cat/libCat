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
valid_cli_chain(int argc, char const* const* argv [[clang::lifetimebound]]) {
   auto cli = cat::cli(argc, argv).add_flag<"n">().add_flag<"f,file">();
   return cli;
}

auto
valid_cli_parse(int argc, char const* const* argv) -> cat::maybe<int4> {
   return cat::cli(argc, argv).add_flag<"n">().parse<int4>("n");
}

// Owning results that do not hold onto the allocator may outlive it.

auto
valid_cli_parse_int_with_allocator(
   cat::span<cat::byte> arena, int argc, char const* const* argv
) -> cat::maybe<int4> {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   return cat::cli(argc, argv)
      .add_flag<"n">()
      .parse<int4>("n", cat::allocator_dyn(linear));
}

auto
valid_cli_parse_str_vec(
   cat::span<cat::byte> arena, int argc, char const* const* argv
) -> cat::maybe<cat::str_vec> {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   return cat::cli(argc, argv)
      .add_flag<"f">()
      .parse<cat::str_vec>("f", cat::allocator_dyn(linear));
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

// Negative cases: a `raii` str vec from `.parse()` outlives its allocator.

auto
dangling_cli_raii_str_vec_allocator_dyn(
   cat::span<cat::byte> arena, int argc, char const* const* argv
) {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+3 1+ {{local variable 'linear'}}
   return cat::cli(argc, argv)
      .add_flag<"f">()
      .parse<cat::raii::str_vec<>>("f", cat::allocator_dyn(linear));
}

auto
dangling_cli_raii_str_vec_allocator(
   cat::span<cat::byte> arena, int argc, char const* const* argv
) {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+4 1+ {{local variable 'linear'}}
   return cat::cli(argc, argv)
      .add_flag<"f">()
      .parse<cat::raii::str_vec<cat::allocator_linear>, cat::allocator_linear>(
         "f", linear
      );
}
