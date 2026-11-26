// Clang `-verify` checks of the `[[gsl::Owner]]`, `[[gsl::Pointer]]`, and
// `[[clang::lifetimebound]]` annotations in `<cat/string>`. Any
// `expected-error` checks must come last, because Clang stops its
// `-Wlifetime-safety` pass after the first error in a translation unit. Run
// with `just test safety`.

#include <cat/allocator_linear>
#include <cat/allocator_page>
#include <cat/random>
#include <cat/scan_str>
#include <cat/string>

using namespace cat::arithmetic;

inline cat::allocator_page g_page;

// Positive cases.

auto
valid_str_view_of_literal() -> cat::str_view {
   return "literal";
}

auto
valid_str_vec_borrow(cat::str_vec& caller_string [[clang::lifetimebound]])
   -> cat::str_view {
   return caller_string;
}

// Negative cases: characters, views, and iterators outlive their string.

auto
dangling_str_inplace_view() -> cat::str_view {
   cat::str_inplace<8u> local = "abc";
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local;
}

auto
dangling_str_vec_begin() -> auto {
   cat::str_vec local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_str_vec_data() -> auto {
   cat::str_vec local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.data();
}

auto
dangling_str_vec_subscript() -> auto& {
   cat::str_vec local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local[0u];
}

auto
dangling_str_vec_front() -> auto& {
   cat::str_vec local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.front();
}

void
use_after_scope_str_vec() {
   char const* p_escaped = nullptr;
   {
      cat::str_vec local;
      // expected-warning@+1 {{local variable 'local' does not live long}}
      p_escaped = local.data();
   }
   auto _ = *p_escaped;
}

auto
dangling_str_inplace_begin() -> auto {
   cat::str_inplace<8u> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_str_inplace_data() -> auto {
   cat::str_inplace<8u> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.data();
}

auto
dangling_str_inplace_subscript() -> auto& {
   cat::str_inplace<8u> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local[0u];
}

auto
dangling_str_inplace_front() -> auto& {
   cat::str_inplace<8u> local;
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.front();
}

void
use_after_scope_str_inplace() {
   char const* p_escaped = nullptr;
   {
      cat::str_inplace<8u> local;
      // expected-warning@+1 {{local variable 'local' does not live long}}
      p_escaped = local.data();
   }
   auto _ = *p_escaped;
}

auto
dangling_raii_str_vec_begin() -> auto {
   cat::raii::str_vec<cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.begin();
}

auto
dangling_raii_str_vec_data() -> auto {
   cat::raii::str_vec<cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.data();
}

auto
dangling_raii_str_vec_subscript() -> auto& {
   cat::raii::str_vec<cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local[0u];
}

auto
dangling_raii_str_vec_front() -> auto& {
   cat::raii::str_vec<cat::allocator_page> local{g_page};
   // expected-warning@+1 1+ {{local variable 'local'}}
   return local.front();
}

void
use_after_scope_raii_str_vec() {
   char const* p_escaped = nullptr;
   {
      cat::raii::str_vec<cat::allocator_page> local{g_page};
      // expected-warning@+1 {{local variable 'local' does not live long}}
      p_escaped = local.data();
   }
   auto _ = *p_escaped;
}

// A generator of letters, because the engines and distributions produce
// integers rather than characters. `wchar_t` is not integral, so wide strings
// take a `letter_generator<unsigned int>`.
template <typename CharT>
struct letter_generator {
   using result_type = CharT;

   static constexpr auto
   min() -> CharT {
      return CharT('a');
   }

   static constexpr auto
   max() -> CharT {
      return CharT('z');
   }

   constexpr auto
   operator()() -> CharT {
      return CharT('a');
   }
};

// Negative cases: a `raii` str vec from a factory or `.clone()`
// outlives its allocator.

auto
dangling_raii_str_vec_allocator_str_vec_empty(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return cat::raii::make_str_vec<cat::allocator_linear>(linear);
}

auto
dangling_raii_str_vec_allocator_str_vec_string(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_str_vec<cat::allocator_linear>(
      linear, cat::str_view("a")
   );
}

auto
dangling_raii_str_vec_allocator_str_vec_strings(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_str_vec<cat::allocator_linear>(
      linear, cat::str_view("a"), cat::str_view("a")
   );
}

auto
dangling_raii_str_vec_allocator_str_vec_reserved(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return cat::raii::make_str_vec_reserved<cat::allocator_linear>(linear, 4u);
}

auto
dangling_raii_str_vec_allocator_str_vec_filled(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_str_vec_filled<cat::allocator_linear>(
      linear, 3u, 'a'
   );
}

auto
dangling_raii_str_vec_allocator_str_vec_filled_random(
   cat::span<cat::byte> arena
) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   letter_generator<char> generator;
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_str_vec_filled_random<cat::allocator_linear>(
      linear, 3u, generator
   );
}

auto
dangling_raii_str_vec_allocator_zstr_vec_empty(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return cat::raii::make_zstr_vec<cat::allocator_linear>(linear);
}

auto
dangling_raii_str_vec_allocator_zstr_vec_string(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_zstr_vec<cat::allocator_linear>(
      linear, cat::str_view("a")
   );
}

auto
dangling_raii_str_vec_allocator_zstr_vec_strings(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_zstr_vec<cat::allocator_linear>(
      linear, cat::str_view("a"), cat::str_view("a")
   );
}

auto
dangling_raii_str_vec_allocator_zstr_vec_reserved(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return cat::raii::make_zstr_vec_reserved<cat::allocator_linear>(linear, 4u);
}

auto
dangling_raii_str_vec_allocator_zstr_vec_filled(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_zstr_vec_filled<cat::allocator_linear>(
      linear, 3u, 'a'
   );
}

auto
dangling_raii_str_vec_allocator_zstr_vec_filled_random(
   cat::span<cat::byte> arena
) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   letter_generator<char> generator;
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_zstr_vec_filled_random<cat::allocator_linear>(
      linear, 3u, generator
   );
}

auto
dangling_raii_str_vec_allocator_wstr_vec_empty(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return cat::raii::make_wstr_vec<cat::allocator_linear>(linear);
}

auto
dangling_raii_str_vec_allocator_wstr_vec_string(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_wstr_vec<cat::allocator_linear>(
      linear, cat::wstr_view(L"a")
   );
}

auto
dangling_raii_str_vec_allocator_wstr_vec_strings(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_wstr_vec<cat::allocator_linear>(
      linear, cat::wstr_view(L"a"), cat::wstr_view(L"a")
   );
}

auto
dangling_raii_str_vec_allocator_wstr_vec_reserved(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return cat::raii::make_wstr_vec_reserved<cat::allocator_linear>(linear, 4u);
}

auto
dangling_raii_str_vec_allocator_wstr_vec_filled(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_wstr_vec_filled<cat::allocator_linear>(
      linear, 3u, L'a'
   );
}

auto
dangling_raii_str_vec_allocator_wstr_vec_filled_random(
   cat::span<cat::byte> arena
) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   letter_generator<unsigned int> generator;
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_wstr_vec_filled_random<cat::allocator_linear>(
      linear, 3u, generator
   );
}

auto
dangling_raii_str_vec_allocator_wzstr_vec_empty(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return cat::raii::make_wzstr_vec<cat::allocator_linear>(linear);
}

auto
dangling_raii_str_vec_allocator_wzstr_vec_string(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_wzstr_vec<cat::allocator_linear>(
      linear, cat::wstr_view(L"a")
   );
}

auto
dangling_raii_str_vec_allocator_wzstr_vec_strings(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_wzstr_vec<cat::allocator_linear>(
      linear, cat::wstr_view(L"a"), cat::wstr_view(L"a")
   );
}

auto
dangling_raii_str_vec_allocator_wzstr_vec_reserved(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return cat::raii::make_wzstr_vec_reserved<cat::allocator_linear>(linear, 4u);
}

auto
dangling_raii_str_vec_allocator_wzstr_vec_filled(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_wzstr_vec_filled<cat::allocator_linear>(
      linear, 3u, L'a'
   );
}

auto
dangling_raii_str_vec_allocator_wzstr_vec_filled_random(
   cat::span<cat::byte> arena
) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   letter_generator<unsigned int> generator;
   // expected-warning@+2 1+ {{local variable 'linear'}}
   return cat::raii::make_wzstr_vec_filled_random<cat::allocator_linear>(
      linear, 3u, generator
   );
}

auto
dangling_raii_str_vec_allocator_str_vec_dyn(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return cat::raii::make_str_vec_filled(cat::allocator_dyn(linear), 3u, 'a');
}

using linear_raii_str_vec =
   __typeof_unqual(cat::raii::make_str_vec<cat::allocator_linear>(
                      cat::declval<cat::allocator_linear&>()
   )
                      .value());

auto
dangling_raii_str_vec_allocator_clone(
   cat::span<cat::byte> arena, linear_raii_str_vec const& source
) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return source.clone(cat::allocator_ref<cat::allocator_linear>(linear));
}

auto
dangling_raii_str_vec_allocator_clone_dyn(
   cat::span<cat::byte> arena, linear_raii_str_vec const& source
) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return source.clone(cat::allocator_dyn(linear));
}

// Negative cases: a result from `scan_str()` outlives its allocator.

auto
dangling_scan_str_int(cat::span<cat::byte> arena) -> cat::maybe<int4> {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return cat::scan_str<int4>(cat::allocator_dyn(linear), "1");
}

auto
dangling_scan_str_inplace(cat::span<cat::byte> arena)
   -> cat::maybe<cat::str_inplace<8u>> {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return cat::scan_str<cat::str_inplace<8u>>(cat::allocator_dyn(linear), "a");
}

auto
dangling_scan_str_vec(cat::span<cat::byte> arena) -> cat::maybe<cat::str_vec> {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return cat::scan_str<cat::str_vec>(cat::allocator_dyn(linear), "a");
}

auto
dangling_raii_str_vec_allocator_scan_str(cat::span<cat::byte> arena) -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+3 1+ {{local variable 'linear'}}
   return cat::scan_str<
      cat::raii::str_vec<cat::allocator_linear>, cat::allocator_linear>(
      linear, "a"
   );
}

auto
dangling_raii_str_vec_allocator_scan_str_dyn(cat::span<cat::byte> arena)
   -> auto {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 1+ {{local variable 'linear'}}
   return cat::scan_str<cat::raii::str_vec<>>(cat::allocator_dyn(linear), "a");
}
