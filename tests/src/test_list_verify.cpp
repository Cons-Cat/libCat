// Clang `-verify` checks of the `[[gsl::Owner]]`, `[[gsl::Pointer]]`, and
// `[[clang::lifetimebound]]` annotations in `<cat/list>`, `<cat/forward_list>`,
// and their `raii` versions. Any `expected-error` checks must come last,
// because Clang stops its `-Wlifetime-safety` pass after the first error in a
// translation unit. Run with `just test safety`.

#include <cat/allocator_linear>
#include <cat/allocator_page>
#include <cat/forward_list>
#include <cat/list>
#include <cat/raii_forward_list>
#include <cat/raii_list>
#include <cat/random>

using namespace cat::arithmetic;

inline cat::allocator_page g_page;

// Positive cases.

auto
valid_list_element_copy() -> int4 {
   cat::list<int4> local;
   return local.front();
}

// Negative cases: element references and iterators outlive their list.

auto
dangling_list_begin() -> auto {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_list_front() -> int4& {
   cat::list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.front();
}

void
use_after_scope_list() {
   int4* p_escaped = nullptr;
   {
      cat::list<int4> local;
      // expected-warning@+1 {{local variable 'local' does not live long}}
      p_escaped = &local.front();
   }
   *p_escaped = 2;
}

auto
dangling_forward_list_begin() -> auto {
   cat::forward_list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_forward_list_front() -> int4& {
   cat::forward_list<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.front();
}

void
use_after_scope_forward_list() {
   int4* p_escaped = nullptr;
   {
      cat::forward_list<int4> local;
      // expected-warning@+1 {{local variable 'local' does not live long}}
      p_escaped = &local.front();
   }
   *p_escaped = 2;
}

auto
dangling_raii_list_begin() -> auto {
   cat::raii::list<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_raii_list_front() -> int4& {
   cat::raii::list<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.front();
}

void
use_after_scope_raii_list() {
   int4* p_escaped = nullptr;
   {
      cat::raii::list<int4, cat::allocator_page> local{g_page};
      // expected-warning@+1 {{local variable 'local' does not live long}}
      p_escaped = &local.front();
   }
   *p_escaped = 2;
}

auto
dangling_raii_forward_list_begin() -> auto {
   cat::raii::forward_list<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_raii_forward_list_front() -> int4& {
   cat::raii::forward_list<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.front();
}

void
use_after_scope_raii_forward_list() {
   int4* p_escaped = nullptr;
   {
      cat::raii::forward_list<int4, cat::allocator_page> local{g_page};
      // expected-warning@+1 {{local variable 'local' does not live long}}
      p_escaped = &local.front();
   }
   *p_escaped = 2;
}

// Negative cases: a `raii` list from a factory or `.clone()`
// outlives its allocator.

auto
dangling_raii_list_allocator_empty(cat::span<cat::byte> arena) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return cat::raii::make_list<cat::uint8, cat::allocator_linear>(linear);
}

auto
dangling_raii_list_allocator_values(cat::span<cat::byte> arena) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_list<cat::uint8, cat::allocator_linear>(
      linear, cat::uint8(1u), cat::uint8(2u)
   );
}

auto
dangling_raii_list_allocator_filled(cat::span<cat::byte> arena) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_list_filled<cat::uint8, cat::allocator_linear>(
      linear, 3u, cat::uint8(1u)
   );
}

auto
dangling_raii_list_allocator_dyn(cat::span<cat::byte> arena) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_list_filled<cat::uint8>(
      cat::allocator_dyn(linear), 3u, cat::uint8(1u)
   );
}

auto
dangling_raii_list_allocator_filled_random(cat::span<cat::byte> arena) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   cat::xoshiro_engine<cat::uint8> engine(1u);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_list_filled_random<cat::uint8, cat::allocator_linear>(
      linear, 3u, engine
   );
}

auto
dangling_raii_list_allocator_filled_random_distribution(
   cat::span<cat::byte> arena
) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   cat::xoshiro_engine<cat::uint8> engine(1u);
   cat::uniform_int_distribution<cat::uint8> distribution(0u, 7u);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_list_filled_random<cat::uint8, cat::allocator_linear>(
      linear, 3u, engine, distribution
   );
}

using linear_raii_list =
   __typeof_unqual(cat::raii::make_list<cat::uint8, cat::allocator_linear>(
                      cat::declval<cat::allocator_linear&>()
   )
                      .value());

auto
dangling_raii_list_allocator_clone(
   cat::span<cat::byte> arena, linear_raii_list const& source
) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return source.clone(cat::allocator_ref<cat::allocator_linear>(linear));
}

auto
dangling_raii_list_allocator_clone_dyn(
   cat::span<cat::byte> arena, linear_raii_list const& source
) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return source.clone(cat::allocator_dyn(linear));
}

// Negative cases: a `raii` forward list from a factory or `.clone()`
// outlives its allocator.

auto
dangling_raii_forward_list_allocator_empty(cat::span<cat::byte> arena) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_forward_list<cat::uint8, cat::allocator_linear>(
      linear
   );
}

auto
dangling_raii_forward_list_allocator_values(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_forward_list<cat::uint8, cat::allocator_linear>(
      linear, cat::uint8(1u), cat::uint8(2u)
   );
}

auto
dangling_raii_forward_list_allocator_filled(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_forward_list_filled<
      cat::uint8, cat::allocator_linear>(linear, 3u, cat::uint8(1u));
}

auto
dangling_raii_forward_list_allocator_dyn(cat::span<cat::byte> arena) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_forward_list_filled<cat::uint8>(
      cat::allocator_dyn(linear), 3u, cat::uint8(1u)
   );
}

auto
dangling_raii_forward_list_allocator_filled_random(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   cat::xoshiro_engine<cat::uint8> engine(1u);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_forward_list_filled_random<
      cat::uint8, cat::allocator_linear>(linear, 3u, engine);
}

auto
dangling_raii_forward_list_allocator_filled_random_distribution(
   cat::span<cat::byte> arena
) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   cat::xoshiro_engine<cat::uint8> engine(1u);
   cat::uniform_int_distribution<cat::uint8> distribution(0u, 7u);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_forward_list_filled_random<
      cat::uint8, cat::allocator_linear>(linear, 3u, engine, distribution);
}

using linear_raii_forward_list = __typeof_unqual(
   cat::raii::make_forward_list<cat::uint8, cat::allocator_linear>(
      cat::declval<cat::allocator_linear&>()
   )
      .value()
);

auto
dangling_raii_forward_list_allocator_clone(
   cat::span<cat::byte> arena, linear_raii_forward_list const& source
) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return source.clone(cat::allocator_ref<cat::allocator_linear>(linear));
}

auto
dangling_raii_forward_list_allocator_clone_dyn(
   cat::span<cat::byte> arena, linear_raii_forward_list const& source
) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return source.clone(cat::allocator_dyn(linear));
}
