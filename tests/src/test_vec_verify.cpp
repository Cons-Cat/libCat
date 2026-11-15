// Clang `-verify` checks of the `[[gsl::Owner]]`, `[[gsl::Pointer]]`, and
// `[[clang::lifetimebound]]` annotations in `<cat/vec>`, `<cat/vec_inplace>`,
// and `<cat/raii_vec>`. Any `expected-error` checks must come last, because
// Clang stops its `-Wlifetime-safety` pass after the first error in a
// translation unit. Run with `just test safety`.

#include <cat/allocator_linear>
#include <cat/allocator_page>
#include <cat/raii_vec>
#include <cat/random>
#include <cat/vec>
#include <cat/vec_inplace>

using namespace cat::arithmetic;

inline cat::allocator_page g_page;

// Positive cases: elements copied out of a local container, and views of a
// container that outlives them.

auto
valid_vec_element_copy() -> int4 {
   cat::vec<int4> local;
   return local[0u];
}

auto
valid_vec_borrow(cat::vec<int4>& caller_vec [[clang::lifetimebound]]) -> int4& {
   return caller_vec[0u];
}

auto
valid_vec_iterator_offset(cat::vec<int4>& caller_vec [[clang::lifetimebound]])
   -> cat::vec<int4>::iterator {
   // An iterator offset from a temporary iterator still refers to the
   // caller's elements.
   return caller_vec.begin() + 1 - 1;
}

// Negative cases: element references and iterators outlive their container.

auto
dangling_vec_iterator_plus() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin() + 1;
}

auto
dangling_vec_iterator_minus() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.end() - 1;
}

auto
dangling_iterator_minus_assign(cat::vec<int4>& caller_vec)
   -> cat::vec<int4>::iterator& {
   cat::vec<int4>::iterator iterator = caller_vec.end();
   // expected-warning@+1 1+ {{local variable 'iterator'}}
   return iterator -= 1;
}

auto
dangling_vec_begin() -> auto {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_vec_data() -> int4* {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.data();
}

auto
dangling_vec_subscript() -> int4& {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local[0u];
}

auto
dangling_vec_front() -> int4& {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.front();
}

auto
dangling_vec_at() -> int4& {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.at(0u).value();
}

auto
dangling_vec_first() -> cat::maybe_span<int4> {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.first(2u);
}

auto
dangling_vec_const_first() -> cat::maybe_span<int4 const> {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::as_const(local).first(2u);
}

auto
dangling_vec_last() -> cat::maybe_span<int4> {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.last(2u);
}

auto
dangling_vec_subspan() -> cat::maybe_span<int4> {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.subspan(0u, 2u);
}

auto
dangling_vec_emplace_back() -> int4& {
   cat::vec<int4> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.emplace_back(g_page, 1).value();
}

void
use_after_scope_vec() {
   int4* p_escaped = nullptr;
   {
      cat::vec<int4> local;
      // expected-warning@+1 {{local variable 'local' does not live long}}
      p_escaped = local.data();
   }
   *p_escaped = 2;
}

auto
dangling_vec_inplace_begin() -> auto {
   cat::vec_inplace<int4, 4u> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_vec_inplace_data() -> int4* {
   cat::vec_inplace<int4, 4u> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.data();
}

auto
dangling_vec_inplace_subscript() -> int4& {
   cat::vec_inplace<int4, 4u> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local[0u];
}

auto
dangling_vec_inplace_front() -> int4& {
   cat::vec_inplace<int4, 4u> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.front();
}

auto
dangling_vec_inplace_at() -> int4& {
   cat::vec_inplace<int4, 4u> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.at(0u).value();
}

auto
dangling_vec_inplace_first() -> cat::maybe_span<int4> {
   cat::vec_inplace<int4, 4u> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.first(2u);
}

auto
dangling_vec_inplace_const_first() -> cat::maybe_span<int4 const> {
   cat::vec_inplace<int4, 4u> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::as_const(local).first(2u);
}

auto
dangling_vec_inplace_last() -> cat::maybe_span<int4> {
   cat::vec_inplace<int4, 4u> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.last(2u);
}

auto
dangling_vec_inplace_subspan() -> cat::maybe_span<int4> {
   cat::vec_inplace<int4, 4u> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.subspan(0u, 2u);
}

auto
dangling_vec_inplace_emplace_back() -> int4& {
   cat::vec_inplace<int4, 4u> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.emplace_back(1).value();
}

auto
dangling_vec_inplace_unchecked_emplace_back() -> int4& {
   cat::vec_inplace<int4, 4u> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.unchecked_emplace_back(1);
}

void
use_after_scope_vec_inplace() {
   int4* p_escaped = nullptr;
   {
      cat::vec_inplace<int4, 4u> local;
      // expected-warning@+1 {{local variable 'local' does not live long}}
      p_escaped = local.data();
   }
   *p_escaped = 2;
}

auto
dangling_raii_vec_begin() -> auto {
   cat::raii::vec<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_raii_vec_data() -> int4* {
   cat::raii::vec<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.data();
}

auto
dangling_raii_vec_subscript() -> int4& {
   cat::raii::vec<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local[0u];
}

auto
dangling_raii_vec_front() -> int4& {
   cat::raii::vec<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.front();
}

auto
dangling_raii_vec_at() -> int4& {
   cat::raii::vec<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.at(0u).value();
}

auto
dangling_raii_vec_first() -> cat::maybe_span<int4> {
   cat::raii::vec<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.first(2u);
}

auto
dangling_raii_vec_const_first() -> cat::maybe_span<int4 const> {
   cat::raii::vec<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return cat::as_const(local).first(2u);
}

auto
dangling_raii_vec_last() -> cat::maybe_span<int4> {
   cat::raii::vec<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.last(2u);
}

auto
dangling_raii_vec_subspan() -> cat::maybe_span<int4> {
   cat::raii::vec<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.subspan(0u, 2u);
}

auto
dangling_raii_vec_emplace_back() -> int4& {
   cat::raii::vec<int4, cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.emplace_back(1).value();
}

void
use_after_scope_raii_vec() {
   int4* p_escaped = nullptr;
   {
      cat::raii::vec<int4, cat::allocator_page> local{g_page};
      // expected-warning@+1 {{local variable 'local' does not live long}}
      p_escaped = local.data();
   }
   *p_escaped = 2;
}

// Negative cases: a `raii` vec from a factory or `.clone()`
// outlives its allocator.

auto
dangling_raii_vec_allocator_empty(cat::span<cat::byte> arena) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return cat::raii::make_vec<cat::uint8, cat::allocator_linear>(linear);
}

auto
dangling_raii_vec_allocator_list(cat::span<cat::byte> arena) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_vec<cat::uint8, cat::allocator_linear>(
      linear, {cat::uint8(1u)}
   );
}

auto
dangling_raii_vec_allocator_reserved(cat::span<cat::byte> arena) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_vec_reserved<cat::uint8, cat::allocator_linear>(
      linear, 4u
   );
}

auto
dangling_raii_vec_allocator_dyn(cat::span<cat::byte> arena) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_vec_filled<cat::uint8>(
      cat::allocator_dyn(linear), 3u, cat::uint8(1u)
   );
}

auto
dangling_raii_vec_allocator_filled(cat::span<cat::byte> arena) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_vec_filled<cat::uint8, cat::allocator_linear>(
      linear, 3u, cat::uint8(1u)
   );
}

auto
dangling_raii_vec_allocator_filled_random(cat::span<cat::byte> arena) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   cat::xoshiro_engine<cat::uint8> engine(1u);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_vec_filled_random<cat::uint8, cat::allocator_linear>(
      linear, 3u, engine
   );
}

auto
dangling_raii_vec_allocator_small_filled(cat::span<cat::byte> arena) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_vec_small_filled<
      cat::uint8, 4u, cat::allocator_linear>(linear, 3u, cat::uint8(1u));
}

auto
dangling_raii_vec_allocator_small_filled_random(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   cat::xoshiro_engine<cat::uint8> engine(1u);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_vec_small_filled_random<
      cat::uint8, 4u, cat::allocator_linear>(linear, 3u, engine);
}

auto
dangling_raii_vec_allocator_big_filled(cat::span<cat::byte> arena) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_vec_big_filled<cat::uint8, cat::allocator_linear>(
      linear, 3u, cat::uint8(1u)
   );
}

auto
dangling_raii_vec_allocator_big_filled_random(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   cat::xoshiro_engine<cat::uint8> engine(1u);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_vec_big_filled_random<
      cat::uint8, cat::allocator_linear>(linear, 3u, engine);
}

auto
dangling_raii_vec_allocator_fixed_filled(cat::span<cat::byte> arena) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_vec_fixed_filled<cat::uint8, cat::allocator_linear>(
      linear, 3u, cat::uint8(1u)
   );
}

auto
dangling_raii_vec_allocator_fixed_filled_random(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   cat::xoshiro_engine<cat::uint8> engine(1u);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_vec_fixed_filled_random<
      cat::uint8, cat::allocator_linear>(linear, 3u, engine);
}

auto
dangling_raii_vec_allocator_small_fixed_filled(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_vec_small_fixed_filled<
      cat::uint8, 4u, cat::allocator_linear>(linear, 3u, cat::uint8(1u));
}

auto
dangling_raii_vec_allocator_small_fixed_filled_random(
   cat::span<cat::byte> arena
) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   cat::xoshiro_engine<cat::uint8> engine(1u);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_vec_small_fixed_filled_random<
      cat::uint8, 4u, cat::allocator_linear>(linear, 3u, engine);
}

auto
dangling_raii_vec_allocator_big_fixed_filled(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_vec_big_fixed_filled<
      cat::uint8, cat::allocator_linear>(linear, 3u, cat::uint8(1u));
}

auto
dangling_raii_vec_allocator_big_fixed_filled_random(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   cat::xoshiro_engine<cat::uint8> engine(1u);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_vec_big_fixed_filled_random<
      cat::uint8, cat::allocator_linear>(linear, 3u, engine);
}

auto
dangling_raii_vec_allocator_filled_random_distribution(
   cat::span<cat::byte> arena
) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   cat::xoshiro_engine<cat::uint8> engine(1u);
   cat::uniform_int_distribution<cat::uint8> distribution(0u, 7u);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_vec_filled_random<cat::uint8, cat::allocator_linear>(
      linear, 3u, engine, distribution
   );
}

using linear_raii_vec = __typeof_unqual(
   cat::raii::make_vec_reserved<cat::uint8, cat::allocator_linear>(
      cat::declval<cat::allocator_linear&>(), 1u
   )
      .value()
);

auto
dangling_raii_vec_allocator_clone(
   cat::span<cat::byte> arena, linear_raii_vec const& source
) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return source.clone(cat::allocator_ref<cat::allocator_linear>(linear));
}

auto
dangling_raii_vec_allocator_clone_dyn(
   cat::span<cat::byte> arena, linear_raii_vec const& source
) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return source.clone(cat::allocator_dyn(linear));
}
