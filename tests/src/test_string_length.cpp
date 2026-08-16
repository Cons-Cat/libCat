#include <cat/iterable>
#include <cat/page_allocator>
#include <cat/string>
#include <cat/utility>

#include "../unit_tests.hpp"

$test(string_length) {
   char const* p_string_1 = "Hello!";
   char const* p_string_7 = "/tmp/temp.sock";
   char const* p_string_with_null = "cat\0tail";
   cat::iword len_1 = cat::string_length(p_string_1);
   cat::iword len_7 = cat::string_length(p_string_7);
   cat::verify(len_1 == 6);
   cat::verify(len_7 == 14);
   cat::str_view string_1 = p_string_1;
   cat::verify(string_1.size() == len_1);
   cat::verify(cat::str_view(p_string_with_null).size() == 3u);

   // This generates a call to `std::wcslen`, which we don't implement yet.
   // wchar_t const* p_wstring = L"wide"
   // wchar_t const* p_wstring_with_null = L"wide\0tail"
   // cat::verify(cat::string_length(p_wstring) == 4u)
   // cat::wstr_view wstring = p_wstring
   // cat::verify(wstring.size() == 4u)
   // cat::verify(cat::wstr_view(p_wstring_with_null).size() == 4u)

   cat::zstr_span mut_zstr = pager.calloc_multi<char>(6).verify();
   mut_zstr[0] = 'a';
   mut_zstr[1] = 'b';
   mut_zstr[2] = 'c';  // A \0 gap is at the 4th byte.
   mut_zstr[4] = 'd';
   $defer {
      pager.free(mut_zstr);
   };
   cat::verify(mut_zstr.size() == 6);
   cat::verify(cat::zstr_view(mut_zstr).size() == 6);

   cat::verify(cat::str_span(mut_zstr).size() == 5);
   cat::verify(cat::str_view(mut_zstr).size() == 5);
   cat::verify(cat::str_view(cat::zstr_view(mut_zstr)).size() == 5);

   auto inplace = cat::make_str_inplace<10u>("Hello");
   cat::verify(inplace.size() == 5);

   auto inplace_2 = cat::make_str_inplace<5u>("Hello");
   cat::verify(inplace_2.size() == 5);

   auto inplace_3 = cat::str_inplace("Hello");
   cat::verify(inplace_3.size() == 5);

   cat::zstr_inplace<10u> inplace_z = cat::make_zstr_inplace<10u>("Hello");
   cat::verify(inplace_z.size() == 5);
   cat::verify(cat::str_span(inplace_z).size() == 5);
   cat::verify(cat::str_view(inplace_z).size() == 5);

   cat::verify(cat::zstr_span(inplace_z).size() == 6);
   cat::verify(cat::zstr_view(inplace_z).size() == 6);

   auto inplace_z_2 = cat::make_zstr_inplace<6u>("Hello");
   cat::verify(inplace_z_2.size() == 5);

   auto inplace_z_3 = inplace_z_2 + inplace_z_2;
   cat::verify(inplace_z_3.size() == 10);

   cat::verify(cat::string_length(L"Hello") == 5u);
   cat::wstr_inplace<5u> wide_inplace = L"Hello";
   cat::verify(wide_inplace.size() == 5u);
   cat::verify(cat::wstr_view(wide_inplace) == cat::wstr_view(L"Hello"));

   cat::wzstr_inplace<5u> wide_inplace_z =
      cat::make_wzstr_inplace<5u>(L"Hello");
   cat::verify(wide_inplace_z.size() == 5u);
   cat::verify(cat::wstr_view(wide_inplace_z).size() == 5u);
   cat::verify(cat::wzstr_view(wide_inplace_z).size() == 6u);

   static constexpr char const char_array[] = {'H', 'i', 'a', '\0'};
   cat::str_inplace arr_inplace = char_array;
   cat::str_inplace arr_inplace2 = cat::make_str_inplace<4>(char_array);
   cat::zstr_inplace arr_zinplac = cat::make_zstr_inplace<4>(char_array);
}

$test(zstr_span_slicing_types) {
   char storage[] = {'a', '\0', 'b', '\0'};
   cat::zstr_span string(storage, 4u);

   auto empty_substring = string.substring(0u, 0u);
   auto full_substring = string.substring(0u, string.size());
   static_assert(cat::is_same<decltype(empty_substring), cat::str_span>);
   static_assert(cat::is_same<decltype(full_substring), cat::str_span>);
   cat::verify(empty_substring.size() == 0u);
   cat::verify(full_substring.size() == string.size());
   cat::verify(full_substring[1u] == '\0');

   auto no_suffix = string.remove_suffix(0u);
   auto full_suffix = string.remove_suffix(string.size());
   static_assert(cat::is_same<decltype(no_suffix), cat::str_span>);
   static_assert(cat::is_same<decltype(full_suffix), cat::str_span>);
   cat::verify(no_suffix.size() == string.size());
   cat::verify(full_suffix.size() == 0u);

   auto no_prefix = string.remove_prefix(0u);
   auto past_interior_null = string.remove_prefix(2u);
   auto full_prefix = string.remove_prefix(string.size());
   static_assert(cat::is_same<decltype(no_prefix), cat::zstr_span>);
   static_assert(cat::is_same<decltype(past_interior_null), cat::zstr_span>);
   static_assert(cat::is_same<decltype(full_prefix), cat::zstr_span>);
   cat::verify(no_prefix.data()[3uz] == '\0');
   cat::verify(past_interior_null.data()[1uz] == '\0');
   cat::verify(full_prefix.size() == 0u);
}

$test(wzstr_span_slicing_types) {
   wchar_t storage[] = {L'a', L'\0', L'b', L'\0'};
   cat::wzstr_span string(storage, 4u);

   auto empty_substring = string.substring(0u, 0u);
   auto full_substring = string.substring(0u, string.size());
   static_assert(cat::is_same<decltype(empty_substring), cat::wstr_span>);
   static_assert(cat::is_same<decltype(full_substring), cat::wstr_span>);
   cat::verify(empty_substring.size() == 0u);
   cat::verify(full_substring.size() == string.size());
   cat::verify(full_substring[1u] == L'\0');

   auto no_suffix = string.remove_suffix(0u);
   auto full_suffix = string.remove_suffix(string.size());
   static_assert(cat::is_same<decltype(no_suffix), cat::wstr_span>);
   static_assert(cat::is_same<decltype(full_suffix), cat::wstr_span>);
   cat::verify(no_suffix.size() == string.size());
   cat::verify(full_suffix.size() == 0u);

   auto no_prefix = string.remove_prefix(0u);
   auto past_interior_null = string.remove_prefix(2u);
   auto full_prefix = string.remove_prefix(string.size());
   static_assert(cat::is_same<decltype(no_prefix), cat::wzstr_span>);
   static_assert(cat::is_same<decltype(past_interior_null), cat::wzstr_span>);
   static_assert(cat::is_same<decltype(full_prefix), cat::wzstr_span>);
   cat::verify(no_prefix.data()[3uz] == L'\0');
   cat::verify(past_interior_null.data()[1uz] == L'\0');
   cat::verify(full_prefix.size() == 0u);
}

$test(string_collection) {
   static_assert(cat::is_random_access_collection<cat::str_inplace<4u>>);
   static_assert(cat::is_random_access_collection<cat::wstr_inplace<4u>>);
   static_assert(cat::is_random_access_collection<cat::str_view>);
   static_assert(cat::is_random_access_collection<cat::wstr_view>);

   cat::str_inplace<3u> text = "cat";
   cat::verify((text | cat::count()) == 3u);
   cat::verify(cat::read_at(text, 0u) == 'c');
   auto non_a_offsets = cat::ref(text)
                           .filter([](char value) -> bool {
                              return value != 'a';
                           })
                           .transform([](char value) -> int {
                              return value - 'a';
                           });
   cat::verify(non_a_offsets.sum() == 21);

   cat::str_view view = text;
   cat::verify((view | cat::count()) == 3u);
   cat::verify(cat::read_at(view, 2u) == 't');
   auto prefix_offsets = cat::ref(view)
                            .filter([](char value) -> bool {
                               return value < 't';
                            })
                            .transform([](char value) -> int {
                               return value - 'a';
                            });
   cat::verify(prefix_offsets.sum() == 2);

   cat::wstr_inplace<3u> wide_text = L"cat";
   cat::verify((wide_text | cat::count()) == 3u);
   cat::verify(cat::read_at(wide_text, 1u) == L'a');

   cat::wstr_view wide_view = wide_text;
   cat::verify((wide_view | cat::count()) == 3u);
   cat::verify(wide_view.find(L't').verify() == 2u);
}
