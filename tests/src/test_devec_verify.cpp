// Clang `-verify` checks of the `[[gsl::Owner]]` and `[[clang::lifetimebound]]`
// annotations in `<cat/devec>` and `<cat/raii_devec>`. Any `expected-error`
// checks must come last, because Clang stops its `-Wlifetime-safety` pass after
// the first error in a translation unit. Run with `just test safety`.

#include <cat/allocator_linear>
#include <cat/allocator_page>
#include <cat/devec>
#include <cat/random>

using namespace cat::arithmetic;

inline cat::allocator_page g_page;

// Positive cases: elements copied out of a local container, and views of a
// container that outlives them.

auto
valid_devec_element_copy() -> int4 {
   cat::devec<int4> local;
   return local[0u];
}

auto
valid_devec_borrow(cat::devec<int4>& caller_devec [[clang::lifetimebound]])
   -> int4& {
   return caller_devec[0u];
}

// Negative cases: element references and iterators outlive their container.

auto
dangling_devec_begin() -> auto {
   cat::devec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_devec_data() -> int4* {
   cat::devec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.data();
}

auto
dangling_devec_subscript() -> int4& {
   cat::devec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local[0u];
}

auto
dangling_devec_front() -> int4& {
   cat::devec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.front();
}

auto
dangling_devec_first() -> cat::maybe_span<int4> {
   cat::devec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.first(2u);
}

auto
dangling_devec_emplace_back() -> int4& {
   cat::devec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.emplace_back(g_page, 1).value();
}

auto
dangling_devec_emplace_front() -> int4& {
   cat::devec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.emplace_front(g_page, 1).value();
}

auto
dangling_devec_emplace() -> int4& {
   cat::devec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.emplace(g_page, 0u, 1).value();
}

void
use_after_scope_devec() {
   int4* p_escaped = nullptr;
   {
      cat::devec<int4> local;
      // expected-warning@+1 {{local variable 'local' does not live long}}
      p_escaped = local.data();
   }
   *p_escaped = 2;
}

auto
dangling_raii_devec_data() -> int4* {
   cat::raii::devec<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.data();
}

auto
dangling_raii_devec_front() -> int4& {
   cat::raii::devec<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.front();
}

auto
dangling_raii_devec_emplace_back() -> int4& {
   cat::raii::devec<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.emplace_back(1).value();
}

auto
dangling_raii_devec_emplace_front() -> int4& {
   cat::raii::devec<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.emplace_front(1).value();
}

auto
dangling_raii_devec_begin() -> auto {
   cat::raii::devec<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

// Negative cases: a `raii::devec` from a factory or `.clone()` outlives its
// allocator.

auto
dangling_raii_devec_allocator_empty(cat::span<cat::byte> arena)
   -> cat::raii::devec<cat::uint8, cat::allocator_linear> {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return cat::raii::make_devec<cat::uint8, cat::allocator_linear>(linear);
}

auto
dangling_raii_devec_allocator_empty_dyn(cat::span<cat::byte> arena)
   -> cat::raii::devec<cat::uint8, cat::allocator_dyn> {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return cat::raii::make_devec<cat::uint8>(cat::allocator_dyn(linear));
}

auto
dangling_raii_devec_allocator_list(cat::span<cat::byte> arena)
   -> cat::maybe<cat::raii::devec<cat::uint8, cat::allocator_linear>> {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_devec<cat::uint8, cat::allocator_linear>(
      linear, {cat::uint8(1u)}
   );
}

auto
dangling_raii_devec_allocator_reserved(cat::span<cat::byte> arena)
   -> cat::maybe<cat::raii::devec<cat::uint8, cat::allocator_linear>> {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_devec_reserved<cat::uint8, cat::allocator_linear>(
      linear, 1u, 1u
   );
}

auto
dangling_raii_devec_allocator_filled(cat::span<cat::byte> arena)
   -> cat::maybe<cat::raii::devec<cat::uint8, cat::allocator_linear>> {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_devec_filled<cat::uint8, cat::allocator_linear>(
      linear, 3u, cat::uint8(1u)
   );
}

auto
dangling_raii_devec_allocator_filled_dyn(cat::span<cat::byte> arena)
   -> cat::maybe<cat::raii::devec<cat::uint8, cat::allocator_dyn>> {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_devec_filled(
      cat::allocator_dyn(linear), 3u, cat::uint8(1u)
   );
}

auto
dangling_raii_devec_allocator_filled_random(cat::span<cat::byte> arena)
   -> cat::maybe<cat::raii::devec<cat::uint8, cat::allocator_linear>> {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   cat::xoshiro_engine<cat::uint8> engine(1u);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_devec_filled_random<
      cat::uint8, cat::allocator_linear>(linear, 3u, engine);
}

auto
dangling_raii_devec_allocator_filled_random_distribution(
   cat::span<cat::byte> arena
) -> cat::maybe<cat::raii::devec<cat::uint8, cat::allocator_linear>> {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   cat::xoshiro_engine<cat::uint8> engine(1u);
   cat::uniform_int_distribution<cat::uint8> distribution(0u, 7u);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_devec_filled_random<
      cat::uint8, cat::allocator_linear>(linear, 3u, engine, distribution);
}

auto
dangling_raii_devec_allocator_clone(
   cat::span<cat::byte> arena,
   cat::raii::devec<cat::uint8, cat::allocator_linear> const& source
) -> cat::maybe<cat::raii::devec<cat::uint8, cat::allocator_linear>> {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return source.clone(cat::allocator_ref<cat::allocator_linear>(linear));
}

auto
dangling_raii_devec_allocator_clone_dyn(
   cat::span<cat::byte> arena,
   cat::raii::devec<cat::uint8, cat::allocator_linear> const& source
) -> cat::maybe<cat::raii::devec<cat::uint8, cat::allocator_dyn>> {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return source.clone(cat::allocator_dyn(linear));
}
