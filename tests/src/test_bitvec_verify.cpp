// Clang `-verify` checks of the `[[gsl::Owner]]`, `[[gsl::Pointer]]`, and
// `[[clang::lifetimebound]]` annotations in `<cat/bitvec>` and
// `<cat/raii_bitvec>`. Any `expected-error` checks must come last, because
// Clang stops its `-Wlifetime-safety` pass after the first error in a
// translation unit. Run with `just test safety`.

#include <cat/allocator_linear>
#include <cat/allocator_page>
#include <cat/bitvec>
#include <cat/raii_bitvec>
#include <cat/random>

using namespace cat::arithmetic;

inline cat::allocator_page g_page;

// Negative cases: bits and iterators outlive their bitvec.

auto
dangling_bitvec_begin() -> auto {
   cat::bitvec local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_bitvec_data() -> auto {
   cat::bitvec local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.data();
}

auto
dangling_raii_bitvec_begin() -> auto {
   cat::raii::bitvec<> local{cat::allocator_dyn(g_page)};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_raii_bitvec_data() -> auto {
   cat::raii::bitvec<> local{cat::allocator_dyn(g_page)};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.data();
}

// Negative cases: a `raii` bitvec from a factory or `.clone()`
// outlives its allocator.

auto
dangling_raii_bitvec_allocator_constructor(cat::span<cat::byte> arena) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return cat::raii::bitvec<cat::allocator_linear>(linear);
}

auto
dangling_raii_bitvec_allocator_dyn(cat::span<cat::byte> arena) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_bitvec_filled(
      cat::allocator_dyn(linear), 3u, cat::bit_value(true)
   );
}

auto
dangling_raii_bitvec_allocator_filled(cat::span<cat::byte> arena) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_bitvec_filled<cat::allocator_linear>(
      linear, 3u, cat::bit_value(true)
   );
}

auto
dangling_raii_bitvec_allocator_filled_random(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   cat::xoshiro_engine<cat::uint8> engine(1u);
   cat::bernoulli_distribution<> distribution(0.5);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_bitvec_filled_random<cat::allocator_linear>(
      linear, 3u, engine, distribution
   );
}

auto
dangling_raii_bitvec_allocator_small_filled(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_bitvec_small_filled<4u, cat::allocator_linear>(
      linear, 3u, cat::bit_value(true)
   );
}

auto
dangling_raii_bitvec_allocator_small_filled_random(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   cat::xoshiro_engine<cat::uint8> engine(1u);
   cat::bernoulli_distribution<> distribution(0.5);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_bitvec_small_filled_random<4u, cat::allocator_linear>(
      linear, 3u, engine, distribution
   );
}

auto
dangling_raii_bitvec_allocator_big_filled(cat::span<cat::byte> arena) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_bitvec_big_filled<cat::allocator_linear>(
      linear, 3u, cat::bit_value(true)
   );
}

auto
dangling_raii_bitvec_allocator_big_filled_random(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   cat::xoshiro_engine<cat::uint8> engine(1u);
   cat::bernoulli_distribution<> distribution(0.5);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_bitvec_big_filled_random<cat::allocator_linear>(
      linear, 3u, engine, distribution
   );
}

auto
dangling_raii_bitvec_allocator_fixed_filled(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_bitvec_fixed_filled<cat::allocator_linear>(
      linear, 3u, cat::bit_value(true)
   );
}

auto
dangling_raii_bitvec_allocator_fixed_filled_random(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   cat::xoshiro_engine<cat::uint8> engine(1u);
   cat::bernoulli_distribution<> distribution(0.5);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_bitvec_fixed_filled_random<cat::allocator_linear>(
      linear, 3u, engine, distribution
   );
}

auto
dangling_raii_bitvec_allocator_small_fixed_filled(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_bitvec_small_fixed_filled<4u, cat::allocator_linear>(
      linear, 3u, cat::bit_value(true)
   );
}

auto
dangling_raii_bitvec_allocator_small_fixed_filled_random(
   cat::span<cat::byte> arena
) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   cat::xoshiro_engine<cat::uint8> engine(1u);
   cat::bernoulli_distribution<> distribution(0.5);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_bitvec_small_fixed_filled_random<
      4u, cat::allocator_linear>(linear, 3u, engine, distribution);
}

auto
dangling_raii_bitvec_allocator_big_fixed_filled(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_bitvec_big_fixed_filled<cat::allocator_linear>(
      linear, 3u, cat::bit_value(true)
   );
}

auto
dangling_raii_bitvec_allocator_big_fixed_filled_random(
   cat::span<cat::byte> arena
) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   cat::xoshiro_engine<cat::uint8> engine(1u);
   cat::bernoulli_distribution<> distribution(0.5);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_bitvec_big_fixed_filled_random<cat::allocator_linear>(
      linear, 3u, engine, distribution
   );
}

using linear_raii_bitvec = __typeof_unqual(
   cat::raii::make_bitvec_filled<cat::allocator_linear>(
      cat::declval<cat::allocator_linear&>(), 1u, cat::bit_value(true)
   )
      .value()
);

auto
dangling_raii_bitvec_allocator_clone(
   cat::span<cat::byte> arena, linear_raii_bitvec const& source
) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return source.clone(cat::allocator_ref<cat::allocator_linear>(linear));
}

auto
dangling_raii_bitvec_allocator_clone_dyn(
   cat::span<cat::byte> arena, linear_raii_bitvec const& source
) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return source.clone(cat::allocator_dyn(linear));
}
