#include <cat/memory>
#include <cat/simd>
#include <cat/string>

#include "../unit_tests.hpp"

namespace {

struct three_byte_word {
   char data[3];

   friend constexpr auto
   operator==(three_byte_word const&, three_byte_word const&) -> bool = default;
};

struct alignas(32) aligned_word {
   cat::uword value;

   friend constexpr auto
   operator==(aligned_word const&, aligned_word const&) -> bool = default;
};

struct sixteen_byte_word {
   char data[16];

   friend constexpr auto
   operator==(sixteen_byte_word const&, sixteen_byte_word const&)
      -> bool = default;
};

constexpr auto
find_memory_constexpr() -> bool {
   constexpr three_byte_word words[] = {
      {{'a', 'b', 'c'}},
      {{'d', 'e', 'f'}},
      {{'g', 'h', 'i'}},
   };
   constexpr three_byte_word needle{
      {'d', 'e', 'f'}
   };
   return cat::find_memory(words, needle, 3u).value() == 1u;
}

constexpr auto
find_subspan_memory_constexpr() -> bool {
   constexpr char haystack[] = "abc abc";
   constexpr char needle[] = "abc";
   return cat::find_subspan_memory(haystack, needle, 7u, 3u).value() == 0u
          && cat::find_subspan_memory(haystack + 1u, needle, 6u, 3u).value()
                == 3u;
}

constexpr auto
string_find_constexpr() -> bool {
   constexpr cat::str_view haystack = "abc abc";
   return haystack.find('b', 2u).value() == 5u
          && haystack.find(cat::str_view{"abc"}, 1u).value() == 4u;
}

static_assert(find_memory_constexpr());
static_assert(find_subspan_memory_constexpr());
static_assert(string_find_constexpr());

}  // namespace

$test(find_memory_basic) {
   char const buffer[] = "abcdef";
   cat::verify(cat::find_memory(buffer, 'a', 6u).value() == 0u);
   cat::verify(cat::find_memory(buffer, 'd', 6u).value() == 3u);
   cat::verify(cat::find_memory(buffer, 'z', 6u).is_empty());
   cat::verify(cat::find_memory(buffer, 'a', 0u).is_empty());
}

$test(find_memory_simd_tail) {
   cat::idx const lanes = cat::native_simd<char>::size();
   char buffer[256]{};
   cat::idx const tail = lanes + 3u;
   buffer[tail] = 'Z';
   cat::verify(cat::find_memory(buffer, 'Z', tail + 1u).value() == tail);
   cat::verify(cat::find_memory(buffer, 'q', tail + 1u).is_empty());
}

$test(find_memory_three_byte_type) {
   three_byte_word const words[] = {
      {{'a', 'b', 'c'}},
      {{'d', 'e', 'f'}},
      {{'g', 'h', 'i'}},
   };
   three_byte_word const hit{
      {'g', 'h', 'i'}
   };
   three_byte_word const miss{
      {'x', 'y', 'z'}
   };
   cat::verify(cat::find_memory(words, hit, 3u).value() == 2u);
   cat::verify(cat::find_memory(words, miss, 3u).is_empty());
}

$test(find_memory_grouped_three_byte_type) {
   three_byte_word words[40]{};
   three_byte_word const hit{
      {'x', 'y', 'z'}
   };
   words[21u] = hit;
   cat::verify(cat::find_memory(words, hit, 40u).value() == 21u);
}

$test(find_memory_sixteen_byte_type) {
   sixteen_byte_word words[10]{};
   sixteen_byte_word const hit{
      {
       'a', 'b',
       'c', 'd',
       'e', 'f',
       'g', 'h',
       'i', 'j',
       'k', 'l',
       'm', 'n',
       'o', 'p',
       },
   };
   words[7u] = hit;
   cat::verify(cat::find_memory(words, hit, 10u).value() == 7u);
}

$test(find_memory_over_aligned_type) {
   aligned_word const words[] = {{11u}, {22u}, {33u}};
   aligned_word const hit{22u};
   aligned_word const miss{44u};
   cat::verify(cat::find_memory(words, hit, 3u).value() == 1u);
   cat::verify(cat::find_memory(words, miss, 3u).is_empty());
}

$test(find_memory_wide_elements) {
   char16_t const chars16[] = u"abcabc";
   char32_t const chars32[] = U"abcabc";
   cat::verify(cat::find_memory(chars16, u'c', 6u).value() == 2u);
   cat::verify(cat::find_memory(chars32, U'b', 6u).value() == 1u);
}

$test(find_subspan_memory_basic) {
   char const haystack[] = "the quick brown fox";
   char const quick[] = "quick";
   char const fox[] = "fox";
   char const miss[] = "cat";
   cat::verify(
      cat::find_subspan_memory(haystack, quick, 19u, 5u).value() == 4u
   );
   cat::verify(cat::find_subspan_memory(haystack, fox, 19u, 3u).value() == 16u);
   cat::verify(cat::find_subspan_memory(haystack, miss, 19u, 3u).is_empty());
}

$test(find_subspan_memory_edge_lengths) {
   char const haystack[] = "abcd";
   char const empty[] = "";
   char const one[] = "c";
   char const long_needle[] = "abcde";
   cat::verify(cat::find_subspan_memory(haystack, empty, 4u, 0u).value() == 0u);
   cat::verify(cat::find_subspan_memory(haystack, one, 4u, 1u).value() == 2u);
   cat::verify(
      cat::find_subspan_memory(haystack, long_needle, 4u, 5u).is_empty()
   );
}

$test(find_subspan_memory_repeated_prefix) {
   char const haystack[] = "aaaaaaaaaaaaaaaaaaaaaaaaabaaaaaaaaaaaaaaaaaaab";
   char const short_needle[] = "aab";
   char const long_needle[] = "aaaab";
   cat::verify(
      cat::find_subspan_memory(haystack, short_needle, 46u, 3u).value() == 23u
   );
   cat::verify(
      cat::find_subspan_memory(haystack, long_needle, 46u, 5u).value() == 21u
   );
}

$test(find_subspan_memory_false_positives_verified) {
   char const haystack[] = "axc axc abc";
   char const needle[] = "abc";
   cat::verify(
      cat::find_subspan_memory(haystack, needle, 11u, 3u).value() == 8u
   );
}

$test(find_subspan_memory_simd_tail) {
   cat::idx const lanes = cat::native_simd<char>::size();
   char haystack[256]{};
   cat::idx const start = lanes * 4u - 2u;
   haystack[start] = 'e';
   haystack[start + 1u] = 'n';
   haystack[start + 2u] = 'd';
   char const needle[] = "end";
   cat::verify(
      cat::find_subspan_memory(haystack, needle, start + 3u, 3u).value()
      == start
   );
}

$test(find_subspan_memory_generic_type) {
   three_byte_word const haystack[] = {
      {{'a', 'a', 'a'}}, {{'b', 'b', 'b'}}, {{'c', 'c', 'c'}},
      {{'b', 'b', 'b'}}, {{'c', 'c', 'c'}},
   };
   three_byte_word const needle[] = {
      {{'b', 'b', 'b'}},
      {{'c', 'c', 'c'}},
   };
   cat::verify(
      cat::find_subspan_memory(haystack, needle, 5u, 2u).value() == 1u
   );
}

$test(find_subspan_memory_grouped_types) {
   three_byte_word three_haystack[40]{};
   three_byte_word const three_needle[] = {
      {{'a', 'b', 'c'}},
      {{'d', 'e', 'f'}},
   };
   three_haystack[21u] = three_needle[0u];
   three_haystack[22u] = three_needle[1u];
   cat::verify(
      cat::find_subspan_memory(three_haystack, three_needle, 40u, 2u).value()
      == 21u
   );

   sixteen_byte_word sixteen_haystack[10]{};
   sixteen_byte_word const sixteen_needle[] = {
      {{'a'}},
      {{'b'}},
   };
   sixteen_haystack[6u] = sixteen_needle[0u];
   sixteen_haystack[7u] = sixteen_needle[1u];
   cat::verify(
      cat::find_subspan_memory(sixteen_haystack, sixteen_needle, 10u, 2u)
         .value()
      == 6u
   );
}

$test(str_view_find_uses_memory_find) {
   cat::str_view const haystack = "abc abc";
   cat::verify(haystack.find('a').value() == 0u);
   cat::verify(haystack.find('a', 1u).value() == 4u);
   cat::verify(haystack.find('z').is_empty());
   cat::verify(haystack.find(cat::str_view{"abc"}, 1u).value() == 4u);
   cat::verify(haystack.find(cat::str_view{""}, 7u).value() == 7u);
   cat::verify(haystack.find(cat::str_view{"a"}, 8u).is_empty());
}

$test(char_family_find_uses_memory_find) {
   char8_t const chars8[] = u8"abcabc";
   char16_t const chars16[] = u"abcabc";
   char32_t const chars32[] = U"abcabc";
   wchar_t const wchars[] = L"abcabc";
   cat::u8str_view const view8{chars8, 6u};
   cat::u16str_view const view16{chars16, 6u};
   cat::u32str_view const view32{chars32, 6u};
   cat::wstr_view const wview{wchars, 6u};
   cat::verify(view8.find(u8'a', 1u).value() == 3u);
   cat::verify(view16.find(u'b', 2u).value() == 4u);
   cat::verify(view32.find(U'c', 3u).value() == 5u);
   cat::verify(wview.find(L'a', 1u).value() == 3u);
}
