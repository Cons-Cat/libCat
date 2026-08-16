#include <cat/array>
#include <cat/iterable>
#include <cat/linear_allocator>
#include <cat/null_allocator>
#include <cat/page_allocator>
#include <cat/string>
#include <cat/vec>

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
   cat::verify(manual_empty.is_empty());
   cat::verify(manual_wide_empty.is_empty());
   cat::verify(raii_empty.is_empty());
   cat::verify(raii_wide_empty.is_empty());
}

$test(string_fill) {
   linear_arena arena;
   auto string = cat::make_str_vec_filled(arena.alloc, 3u, 'a').verify();
   auto zstring = cat::make_zstr_vec_filled(arena.alloc, 3u, 'b').verify();
   auto wide = cat::make_wstr_vec_filled(arena.alloc, 3u, L'c').verify();
   auto raii = cat::raii::make_str_vec_filled(arena.alloc, 3u, 'd').verify();

   string.fill('e');
   zstring.fill('f');
   wide.fill(L'g');
   raii.fill('h');
   cat::verify(string == "eee");
   cat::verify(zstring == "fff");
   cat::verify(wide == L"ggg");
   cat::verify(raii == "hhh");

   string.free(arena.alloc);
   zstring.free(arena.alloc);
   wide.free(arena.alloc);

   char text[] = "cat";
   cat::str_span span(text, 3u);
   cat::zstr_span zspan(text, 4u);
   span.fill('x');
   cat::verify(cat::str_view(text, 3u) == "xxx");
   zspan.fill('y');
   cat::verify(cat::str_view(text, 3u) == "yyy");
   cat::verify(text[3] == '\0');
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

   cat::verify(zstring.size() == 3u);
   cat::verify(zstring.data()[zstring.size()] == '\0');
   cat::verify(cat::str_view(zstring) == cat::str_view("cat"));

   zstring.append(arena.alloc, cat::zstr_view("s")).verify();
   cat::verify(zstring.size() == 4u);
   cat::verify(zstring.data()[zstring.size()] == '\0');
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

   cat::verify(wide_zstring.size() == 3u);
   cat::verify(wide_zstring.data()[wide_zstring.size()] == L'\0');
   cat::verify(cat::wstr_view(wide_zstring) == cat::wstr_view(L"cat"));

   wide_zstring.append(arena.alloc, cat::wzstr_view(L"s")).verify();
   cat::verify(wide_zstring.size() == 4u);
   cat::verify(wide_zstring.data()[wide_zstring.size()] == L'\0');
   cat::verify(cat::wstr_view(wide_zstring) == cat::wstr_view(L"cats"));
}

$test(str_vec_append_range_variants) {
   linear_arena arena;
   cat::array<char, 3u> narrow{'c', 'a', 't'};
   cat::array<wchar_t, 3u> wide{L'c', L'a', L't'};

   auto verify_manual_narrow = [&]<typename Vector> {
      Vector string;
      string.append_range(arena.alloc, narrow).verify();
      string.append_range(arena.alloc, narrow).verify();
      cat::verify(string.view() == cat::str_view("catcat"));
      string.free(arena.alloc);
   };
   verify_manual_narrow.template operator()<cat::str_vec>();
   verify_manual_narrow.template operator()<cat::zstr_vec>();

   auto verify_manual_wide = [&]<typename Vector> {
      Vector string;
      string.append_range(arena.alloc, wide).verify();
      string.append_range(arena.alloc, wide).verify();
      cat::verify(string.view() == cat::wstr_view(L"catcat"));
      string.free(arena.alloc);
   };
   verify_manual_wide.template operator()<cat::wstr_vec>();
   verify_manual_wide.template operator()<cat::wzstr_vec>();

   auto verify_raii_narrow = [&]<typename Vector> {
      Vector string{cat::dyn_allocator(arena.alloc)};
      string.append_range(narrow).verify();
      string.append_range(narrow).verify();
      cat::verify(string.view() == cat::str_view("catcat"));
   };
   verify_raii_narrow.template operator()<cat::raii::str_vec<>>();
   verify_raii_narrow.template operator()<cat::raii::zstr_vec<>>();

   auto verify_raii_wide = [&]<typename Vector> {
      Vector string{cat::dyn_allocator(arena.alloc)};
      string.append_range(wide).verify();
      string.append_range(wide).verify();
      cat::verify(string.view() == cat::wstr_view(L"catcat"));
   };
   verify_raii_wide.template operator()<cat::raii::wstr_vec<>>();
   verify_raii_wide.template operator()<cat::raii::wzstr_vec<>>();
}

$test(str_vec_iterable_range_modifiers) {
   linear_arena arena;
   cat::array<char, 3u> source{'c', 'a', 't'};
   cat::raii::zstr_vec<cat::linear_allocator> string(arena.alloc);
   auto consonants = cat::ref(source).filter([](char value) -> bool {
      return value != 'a';
   });
   string.append_range(consonants).verify();
   cat::verify(string.view() == cat::str_view("ct"));

   string.insert_range(1u, source).verify();
   cat::verify(string.view() == cat::str_view("ccatt"));

   auto vowels = cat::ref(source).filter([](char value) -> bool {
      return value == 'a';
   });
   string.replace_with_range(1u, 4u, vowels).verify();
   cat::verify(string.view() == cat::str_view("cat"));
   cat::verify(string.data()[string.size()] == '\0');
}

$test(str_vec_contiguous_range_modifiers) {
   linear_arena arena;
   cat::array<char, 3u> source{'c', 'a', 't'};
   cat::array<char, 2u> insertion{'o', 'w'};
   cat::array<char, 1u> replacement{'!'};
   cat::raii::zstr_vec<cat::linear_allocator> string(arena.alloc);

   string.append_range(source).verify();
   string.insert_range(1u, insertion).verify();
   string.replace_with_range(2u, 4u, replacement).verify();

   cat::verify(string.view() == cat::str_view("co!t"));
   cat::verify(string.data()[string.size()] == '\0');
}

$test(str_vec_variadic_concat) {
   linear_arena arena;

   cat::str_inplace inplace = "c";
   cat::zstr_inplace<1u> z_inplace = cat::make_zstr_inplace<1u>("d");
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
   cat::verify(z_concatenated.size() == 6u);
   cat::verify(z_concatenated.data()[z_concatenated.size()] == '\0');
   cat::verify(z_concatenated.view() == cat::str_view("abcdef"));

   cat::raii::str_vec managed_concatenated =
      cat::raii::make_str_vec(
         arena.alloc, cat::str_view("a"), cat::zstr_view("b"), inplace,
         z_inplace, manual, managed
      )
         .verify();
   cat::verify(managed_concatenated == cat::str_view("abcdef"));

   cat::wstr_inplace wide_inplace = L"c";
   cat::wzstr_inplace<1u> wide_z_inplace = cat::make_wzstr_inplace<1u>(L"d");
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
   cat::verify(wide_managed_concatenated.size() == 6u);
   cat::verify(
      wide_managed_concatenated.data()[wide_managed_concatenated.size()]
      == L'\0'
   );
   cat::verify(wide_managed_concatenated.view() == cat::wstr_view(L"abcdef"));
}

$test(zstr_vec_mutation_preserves_terminator) {
   linear_arena arena;
   cat::zstr_vec string;
   $defer {
      string.free(arena.alloc);
   };

   cat::verify(string.size() == 0u);

   string.push_back(arena.alloc, 'a').verify();
   string.push_back(arena.alloc, 'b').verify();
   cat::verify(string.size() == 2u);
   cat::verify(string.data()[string.size()] == '\0');
   cat::verify(cat::str_view(string) == cat::str_view("ab"));

   cat::verify(string.pop_back().verify() == 'b');
   cat::verify(string.size() == 1u);
   cat::verify(string.data()[string.size()] == '\0');

   string.erase(0u);
   cat::verify(string.size() == 0u);
   cat::verify(string.data()[string.size()] == '\0');

   string.append(arena.alloc, cat::str_view("xyz")).verify();
   string.erase(1u, 3u);
   cat::verify(cat::str_view(string) == cat::str_view("x"));
   cat::verify(string.data()[string.size()] == '\0');

   string.clear();
   cat::verify(string.size() == 0u);
   cat::verify(string.data()[string.size()] == '\0');
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
   cat::verify(failed.push_back(null_alloc, 'x').is_empty());
   cat::zstr_vec zfailed;
   cat::verify(zfailed.push_back(null_alloc, 'x').is_empty());

   cat::wstr_vec wide_failed;
   $defer {
      wide_failed.free(null_alloc);
   };
   cat::verify(wide_failed.push_back(null_alloc, L'x').is_empty());
   cat::wzstr_vec wzfailed;
   cat::verify(wzfailed.push_back(null_alloc, L'x').is_empty());
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

   cat::verify(string.size() == 0u);
   cat::verify(string.data()[0u] == '\0');

   string.append(cat::str_view("hi")).verify();
   cat::verify(string.size() == 2u);
   cat::verify(string.data()[string.size()] == '\0');
   cat::verify(string.view() == cat::str_view("hi"));

   cat::raii::zstr_vec filled =
      cat::raii::make_zstr_vec_filled(arena.alloc, 4u, 'x').verify();
   cat::verify(filled.size() == 4u);
   cat::verify(filled.data()[filled.size()] == '\0');
   cat::verify(filled.view() == cat::str_view("xxxx"));

   filled.clear();
   cat::verify(filled.size() == 0u);
   cat::verify(filled.data()[filled.size()] == '\0');

   cat::raii::wzstr_vec wide_string =
      cat::raii::make_wzstr_vec(arena.alloc).verify();

   cat::verify(wide_string.size() == 0u);
   cat::verify(wide_string.data()[0u] == L'\0');

   wide_string.append(cat::wstr_view(L"hi")).verify();
   cat::verify(wide_string.size() == 2u);
   cat::verify(wide_string.data()[wide_string.size()] == L'\0');
   cat::verify(wide_string.view() == cat::wstr_view(L"hi"));

   cat::raii::wzstr_vec wide_filled =
      cat::raii::make_wzstr_vec_filled(arena.alloc, 4u, L'x').verify();
   cat::verify(wide_filled.size() == 4u);
   cat::verify(wide_filled.data()[wide_filled.size()] == L'\0');
   cat::verify(wide_filled.view() == cat::wstr_view(L"xxxx"));
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
