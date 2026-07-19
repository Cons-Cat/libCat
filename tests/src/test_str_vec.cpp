#include <cat/iterable>
#include <cat/linear_allocator>
#include <cat/null_allocator>
#include <cat/string>

#include "../unit_tests.hpp"

namespace {

struct linear_arena {
   cat::span<cat::byte> page = pager.alloc_multi<cat::byte>(4_uki).verify();
   cat::linear_allocator alloc = cat::make_linear_allocator(page);

   ~linear_arena() {
      pager.free(page);
   }
};

}  // namespace

$test(str_vec_maybe_niche) {
   static_assert(sizeof(cat::maybe<cat::str_vec>) == sizeof(cat::str_vec));
   static_assert(sizeof(cat::maybe<cat::zstr_vec>) == sizeof(cat::zstr_vec));
   static_assert(sizeof(cat::maybe<cat::wstr_vec>) == sizeof(cat::wstr_vec));
   static_assert(sizeof(cat::maybe<cat::wzstr_vec>) == sizeof(cat::wzstr_vec));
   static_assert(
      sizeof(cat::maybe<cat::raii::str_vec<>>) == sizeof(cat::raii::str_vec<>)
   );
   static_assert(
      sizeof(cat::maybe<cat::raii::zstr_vec<>>) == sizeof(cat::raii::zstr_vec<>)
   );
   static_assert(
      sizeof(cat::maybe<cat::raii::wstr_vec<>>) == sizeof(cat::raii::wstr_vec<>)
   );
   static_assert(
      sizeof(cat::maybe<cat::raii::wzstr_vec<>>)
      == sizeof(cat::raii::wzstr_vec<>)
   );

   cat::maybe<cat::str_vec> manual_empty;
   cat::maybe<cat::wstr_vec> manual_wide_empty;
   cat::maybe<cat::raii::str_vec<>> raii_empty;
   cat::maybe<cat::raii::wstr_vec<>> raii_wide_empty;
   cat::verify(!manual_empty.has_value());
   cat::verify(!manual_wide_empty.has_value());
   cat::verify(!raii_empty.has_value());
   cat::verify(!raii_wide_empty.has_value());
}

$test(str_vec_make_append_and_views) {
   linear_arena arena;
   cat::str_vec string =
      cat::make_str_vec(arena.alloc, cat::str_view("cat")).verify();
   $defer {
      string.free(arena.alloc);
   };

   cat::verify(string.size() == 3u);
   cat::verify(string == cat::str_view("cat"));
   cat::str_span mutable_span = string;
   mutable_span[0] = 'b';
   cat::verify(string == cat::str_view("bat"));

   string.append(arena.alloc, cat::str_view("s")).verify();
   cat::verify(string == cat::str_view("bats"));

   cat::zstr_vec zstring =
      cat::make_zstr_vec(arena.alloc, cat::str_view("cat")).verify();
   $defer {
      zstring.free(arena.alloc);
   };

   cat::verify(zstring.size() == 4u);
   cat::verify(zstring[3u] == '\0');
   cat::verify(cat::str_view(zstring) == cat::str_view("cat"));

   zstring.append(arena.alloc, cat::zstr_view("s")).verify();
   cat::verify(zstring.size() == 5u);
   cat::verify(zstring[4u] == '\0');
   cat::verify(cat::str_view(zstring) == cat::str_view("cats"));

   cat::wstr_vec wide_string =
      cat::make_wstr_vec(arena.alloc, cat::wstr_view(L"cat")).verify();
   $defer {
      wide_string.free(arena.alloc);
   };

   cat::verify(wide_string.size() == 3u);
   cat::verify(wide_string == cat::wstr_view(L"cat"));
   cat::wstr_span mutable_wide_span = wide_string;
   mutable_wide_span[0] = L'b';
   cat::verify(wide_string == cat::wstr_view(L"bat"));

   wide_string.append(arena.alloc, cat::wstr_view(L"s")).verify();
   cat::verify(wide_string == cat::wstr_view(L"bats"));

   cat::wzstr_vec wide_zstring =
      cat::make_wzstr_vec(arena.alloc, cat::wstr_view(L"cat")).verify();
   $defer {
      wide_zstring.free(arena.alloc);
   };

   cat::verify(wide_zstring.size() == 4u);
   cat::verify(wide_zstring[3u] == L'\0');
   cat::verify(cat::wstr_view(wide_zstring) == cat::wstr_view(L"cat"));

   wide_zstring.append(arena.alloc, cat::wzstr_view(L"s")).verify();
   cat::verify(wide_zstring.size() == 5u);
   cat::verify(wide_zstring[4u] == L'\0');
   cat::verify(cat::wstr_view(wide_zstring) == cat::wstr_view(L"cats"));
}

$test(str_vec_variadic_concat) {
   linear_arena arena;

   cat::str_inplace inplace = "c";
   cat::zstr_inplace<2u> z_inplace = cat::make_zstr_inplace<2u>("d");
   cat::str_vec manual =
      cat::make_str_vec(arena.alloc, cat::str_view("e")).verify();
   $defer {
      manual.free(arena.alloc);
   };
   cat::raii::zstr_vec managed =
      cat::raii::make_zstr_vec(arena.alloc, cat::str_view("f")).verify();

   cat::str_vec concatenated =
      cat::make_str_vec(
         arena.alloc, cat::str_view("a"), cat::zstr_view("b"), inplace,
         z_inplace, manual, managed
      )
         .verify();
   $defer {
      concatenated.free(arena.alloc);
   };
   cat::verify(concatenated == cat::str_view("abcdef"));

   cat::zstr_vec z_concatenated =
      cat::make_zstr_vec(
         arena.alloc, cat::str_view("a"), cat::zstr_view("b"), inplace,
         z_inplace, manual, managed
      )
         .verify();
   $defer {
      z_concatenated.free(arena.alloc);
   };
   cat::verify(z_concatenated.size() == 7u);
   cat::verify(z_concatenated[6u] == '\0');
   cat::verify(z_concatenated.view() == cat::str_view("abcdef"));

   cat::raii::str_vec managed_concatenated =
      cat::raii::make_str_vec(
         arena.alloc, cat::str_view("a"), cat::zstr_view("b"), inplace,
         z_inplace, manual, managed
      )
         .verify();
   cat::verify(managed_concatenated == cat::str_view("abcdef"));

   cat::wstr_inplace wide_inplace = L"c";
   cat::wzstr_inplace<2u> wide_z_inplace = cat::make_wzstr_inplace<2u>(L"d");
   cat::wstr_vec wide_manual =
      cat::make_wstr_vec(arena.alloc, cat::wstr_view(L"e")).verify();
   $defer {
      wide_manual.free(arena.alloc);
   };
   cat::raii::wzstr_vec wide_managed =
      cat::raii::make_wzstr_vec(arena.alloc, cat::wstr_view(L"f")).verify();

   cat::wstr_vec wide_concatenated =
      cat::make_wstr_vec(
         arena.alloc, cat::wstr_view(L"a"), cat::wzstr_view(L"b"), wide_inplace,
         wide_z_inplace, wide_manual, wide_managed
      )
         .verify();
   $defer {
      wide_concatenated.free(arena.alloc);
   };
   cat::verify(wide_concatenated == cat::wstr_view(L"abcdef"));

   cat::raii::wzstr_vec wide_managed_concatenated =
      cat::raii::make_wzstr_vec(
         arena.alloc, cat::wstr_view(L"a"), cat::wzstr_view(L"b"), wide_inplace,
         wide_z_inplace, wide_manual, wide_managed
      )
         .verify();
   cat::verify(wide_managed_concatenated.size() == 7u);
   cat::verify(wide_managed_concatenated[6u] == L'\0');
   cat::verify(wide_managed_concatenated.view() == cat::wstr_view(L"abcdef"));
}

$test(zstr_vec_mutation_preserves_terminator) {
   linear_arena arena;
   cat::zstr_vec string = cat::make_zstr_vec(arena.alloc).verify();
   $defer {
      string.free(arena.alloc);
   };

   cat::verify(string.size() == 1u);
   cat::verify(string[0u] == '\0');

   string.push_back(arena.alloc, 'a').verify();
   string.push_back(arena.alloc, 'b').verify();
   cat::verify(string.size() == 3u);
   cat::verify(string[2u] == '\0');
   cat::verify(cat::str_view(string) == cat::str_view("ab"));

   cat::verify(string.pop_back().verify() == 'b');
   cat::verify(string.size() == 2u);
   cat::verify(string[1u] == '\0');

   string.erase(0u);
   cat::verify(string.size() == 1u);
   cat::verify(string[0u] == '\0');

   string.append(arena.alloc, cat::str_view("xyz")).verify();
   string.erase(1u, 3u);
   cat::verify(cat::str_view(string) == cat::str_view("x"));
   cat::verify(string[1u] == '\0');

   string.clear();
   cat::verify(string.size() == 1u);
   cat::verify(string[0u] == '\0');
}

$test(str_vec_clone_move_and_failure) {
   linear_arena arena;
   cat::str_vec source =
      cat::make_str_vec(arena.alloc, cat::str_view("copy")).verify();
   $defer {
      source.free(arena.alloc);
   };

   cat::str_vec cloned = source.clone(arena.alloc).verify();
   $defer {
      cloned.free(arena.alloc);
   };
   cat::verify(cloned == cat::str_view("copy"));

   cat::str_vec moved = cat::move(cloned);
   $defer {
      moved.free(arena.alloc);
   };
   cat::verify(moved == cat::str_view("copy"));
   cat::verify(cloned.data() == nullptr);

   cat::null_allocator null_alloc;
   cat::str_vec failed;
   $defer {
      failed.free(null_alloc);
   };
   cat::verify(!failed.push_back(null_alloc, 'x').has_value());
   cat::verify(!cat::make_zstr_vec(null_alloc).has_value());

   cat::wstr_vec wide_failed;
   $defer {
      wide_failed.free(null_alloc);
   };
   cat::verify(!wide_failed.push_back(null_alloc, L'x').has_value());
   cat::verify(!cat::make_wzstr_vec(null_alloc).has_value());
}

$test(raii_str_vec_lifecycle) {
   linear_arena arena;
   cat::raii::str_vec string =
      cat::raii::make_str_vec(arena.alloc, cat::str_view("ra")).verify();

   string.push_back('i').verify();
   string.append(cat::str_view("i")).verify();
   cat::verify(string == cat::str_view("raii"));

   cat::raii::str_vec cloned = string.clone(arena.alloc).verify();
   cat::verify(cloned == cat::str_view("raii"));

   cat::str_vec released = cat::move(cloned).release();
   $defer {
      released.free(arena.alloc);
   };
   cat::verify(released == cat::str_view("raii"));

   string.clear();
   cat::verify(string.size() == 0u);
   string.reset();
   cat::verify(string.data() == nullptr);

   cat::raii::wstr_vec wide_string =
      cat::raii::make_wstr_vec(arena.alloc, cat::wstr_view(L"ra")).verify();
   wide_string.push_back(L'i').verify();
   wide_string.append(cat::wstr_view(L"i")).verify();
   cat::verify(wide_string == cat::wstr_view(L"raii"));

   cat::raii::wstr_vec wide_cloned = wide_string.clone(arena.alloc).verify();
   cat::verify(wide_cloned == cat::wstr_view(L"raii"));

   cat::wstr_vec wide_released = cat::move(wide_cloned).release();
   $defer {
      wide_released.free(arena.alloc);
   };
   cat::verify(wide_released == cat::wstr_view(L"raii"));
}

$test(raii_zstr_vec_lifecycle) {
   linear_arena arena;
   cat::raii::zstr_vec string = cat::raii::make_zstr_vec(arena.alloc).verify();

   cat::verify(string.size() == 1u);
   cat::verify(string[0u] == '\0');

   string.append(cat::str_view("hi")).verify();
   cat::verify(string.size() == 3u);
   cat::verify(string[2u] == '\0');
   cat::verify(string.view() == cat::str_view("hi"));

   cat::raii::zstr_vec filled =
      cat::raii::make_zstr_vec_filled(arena.alloc, 4u, 'x').verify();
   cat::verify(filled.size() == 4u);
   cat::verify(filled[3u] == '\0');
   cat::verify(filled.view() == cat::str_view("xxx"));

   filled.clear();
   cat::verify(filled.size() == 1u);
   cat::verify(filled[0u] == '\0');

   cat::raii::wzstr_vec wide_string =
      cat::raii::make_wzstr_vec(arena.alloc).verify();

   cat::verify(wide_string.size() == 1u);
   cat::verify(wide_string[0u] == L'\0');

   wide_string.append(cat::wstr_view(L"hi")).verify();
   cat::verify(wide_string.size() == 3u);
   cat::verify(wide_string[2u] == L'\0');
   cat::verify(wide_string.view() == cat::wstr_view(L"hi"));

   cat::raii::wzstr_vec wide_filled =
      cat::raii::make_wzstr_vec_filled(arena.alloc, 4u, L'x').verify();
   cat::verify(wide_filled.size() == 4u);
   cat::verify(wide_filled[3u] == L'\0');
   cat::verify(wide_filled.view() == cat::wstr_view(L"xxx"));
}

$test(str_vec_collection) {
   static_assert(cat::is_random_access_collection<cat::str_vec>);
   static_assert(cat::is_random_access_collection<cat::zstr_vec>);
   static_assert(cat::is_random_access_collection<cat::wstr_vec>);
   static_assert(cat::is_random_access_collection<cat::wzstr_vec>);
   static_assert(cat::is_random_access_collection<cat::raii::str_vec<>>);
   static_assert(cat::is_random_access_collection<cat::raii::zstr_vec<>>);
   static_assert(cat::is_random_access_collection<cat::raii::wstr_vec<>>);
   static_assert(cat::is_random_access_collection<cat::raii::wzstr_vec<>>);

   linear_arena arena;
   cat::str_vec string =
      cat::make_str_vec(arena.alloc, cat::str_view("cat")).verify();
   $defer {
      string.free(arena.alloc);
   };

   cat::verify((string | cat::count()) == 3u);
   cat::verify(cat::read_at(string, 2u) == 't');

   cat::wstr_vec wide_string =
      cat::make_wstr_vec(arena.alloc, cat::wstr_view(L"cat")).verify();
   $defer {
      wide_string.free(arena.alloc);
   };

   cat::verify((wide_string | cat::count()) == 3u);
   cat::verify(cat::read_at(wide_string, 2u) == L't');
}
