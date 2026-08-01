#include <cat/iterable>
#include <cat/string>

#include "../unit_tests.hpp"

template <cat::basic_str_literal text>
struct literal_tag {
   static constexpr auto value = text;
};

$test(str_literal_construction_and_access) {
   constexpr cat::basic_str_literal text = "cat";
   constexpr cat::basic_str_literal packed{'c', 'a', 't'};
   constexpr cat::basic_str_literal empty = "";

   static_assert(text == packed);
   static_assert(text == "cat");
   static_assert(text.size() == 3u);
   static_assert(text.length() == 3u);
   static_assert(text.max_size() == 3u);
   static_assert(!text.empty());
   static_assert(empty.empty());
   static_assert(text.front() == 'c');
   static_assert(text.back() == 't');
   static_assert(text.at(1u) == 'a');
   static_assert(text.c_str()[3] == '\0');
   static_assert(text.data() == text.c_str());
   static_assert(text.view() == cat::str_view("cat"));
   static_assert(cat::is_same<decltype(text.data()), char const*>);
   static_assert(!cat::is_default_constructible<cat::str_literal<0>>);
   static_assert(!cat::is_default_constructible<cat::str_literal<3>>);

   constexpr cat::array characters{'c', 'a', 't'};
   constexpr cat::basic_str_literal from_array(characters);
   static_assert(from_array == text);

   char const range[] = {'c', 'a', 't'};
   cat::str_literal<3> from_range(range, range + 3u);
   cat::verify(from_range == text);

   cat::idx count;
   for (char character : text) {
      cat::verify(character == text[count]);
      ++count;
   }
   cat::verify(count == text.size());

   auto consonant_offsets = text
                               .filter([](char value) -> bool {
                                  return value != 'a';
                               })
                               .transform([](char value) -> int {
                                  return value - 'a';
                               });
   cat::verify(consonant_offsets.sum() == 21);
}

$test(str_literal_concat_compare_and_swap) {
   constexpr cat::basic_str_literal hello = "Hello";
   constexpr cat::basic_str_literal world = "world";
   constexpr auto joined = hello + ", " + world + '!';
   constexpr auto wrapped = '[' + cat::basic_str_literal("cat") + "]";

   static_assert(joined == "Hello, world!");
   static_assert(wrapped == "[cat]");
   static_assert(cat::basic_str_literal("abc") < "abd");
   static_assert("abb" < cat::basic_str_literal("abc"));
   static_assert(
      cat::basic_str_literal("abc") < cat::basic_str_literal("abcd")
   );

   cat::basic_str_literal left = "abc";
   cat::basic_str_literal right = "xyz";
   cat::swap(left, right);
   cat::verify(left == cat::basic_str_literal("xyz"));
   cat::verify(right == cat::basic_str_literal("abc"));
}

$test(str_literal_character_aliases) {
   constexpr cat::basic_str_literal utf8 = u8"cat";
   constexpr cat::basic_str_literal utf16 = u"cat";
   constexpr cat::basic_str_literal utf32 = U"cat";
   constexpr cat::basic_str_literal wide = L"cat";

   static_assert(cat::is_same<decltype(utf8), cat::u8str_literal<3> const>);
   static_assert(cat::is_same<decltype(utf16), cat::u16str_literal<3> const>);
   static_assert(cat::is_same<decltype(utf32), cat::u32str_literal<3> const>);
   static_assert(cat::is_same<decltype(wide), cat::wstr_literal<3> const>);
}

$test(str_literal_wide_char) {
   constexpr cat::basic_str_literal text = L"wide";
   constexpr cat::basic_str_literal packed{L'w', L'i', L'd', L'e'};
   constexpr auto wrapped = L'[' + text + L"]";

   static_assert(text == packed);
   static_assert(text.size() == 4u);
   static_assert(text.front() == L'w');
   static_assert(text.back() == L'e');
   static_assert(text.c_str()[4] == L'\0');
   static_assert(text.view() == cat::wstr_view(L"wide"));
   static_assert(wrapped == L"[wide]");
   static_assert(cat::basic_str_literal(L"wide") < L"wider");

   using tagged = literal_tag<L"compile time">;
   static_assert(tagged::value == L"compile time");
   static_assert(cat::is_structural<cat::wstr_literal<4>>);
}

$test(str_literal_structural_nttp) {
   using tagged = literal_tag<"compile time">;
   static_assert(tagged::value == "compile time");
   static_assert(tagged::value.data_[8] == 't');
   static_assert(cat::is_structural<cat::str_literal<4>>);
}
