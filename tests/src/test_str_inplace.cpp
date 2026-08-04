#include <cat/string>

#include "../unit_tests.hpp"

$test(str_inplace_construct_and_concat) {
   constexpr cat::str_inplace const_string_3 = "Hello, ";
   constexpr cat::str_inplace const_string_4 = "world!";

   cat::verify(const_string_3.at(10).is_empty());

   // TODO: Make this `constexpr`.
   cat::str_inplace hello_world = (const_string_3 + const_string_4);
   constexpr cat::str_inplace const_hello_world =
      (const_string_3 + const_string_4);

   constexpr cat::str_inplace<13> const_hello_world_5 =
      cat::str_inplace("Hello, ") + "world!";
   constexpr cat::zstr_inplace<13> const_hello_world_6 =
      cat::make_zstr_inplace<7>("Hello, ") + "world!";
   constexpr cat::str_inplace const_hello_world_7 =
      "Hello, " + cat::str_inplace("world!");
   cat::zstr_inplace<13> const_hello_world_8 =
      "Hello, " + cat::make_zstr_inplace<6>("world!");
   static_assert(const_hello_world_5 == "Hello, world!");
   static_assert(const_hello_world_6 == "Hello, world!");

   cat::verify(hello_world == const_hello_world);
   cat::verify(const_hello_world_7 == "Hello, world!");
   static_assert((cat::str_inplace("cat") + '!') == "cat!");
   static_assert(('>' + cat::str_inplace("cat")) == ">cat");
   static_assert(
      cat::str_view(cat::str_inplace("cat")) == cat::str_view("cat")
   );
}

$test(str_inplace_compare_to_char) {
   cat::str_inplace char_str = "X";
   cat::verify(char_str == 'X');

   cat::zstr_inplace char_zstr = cat::make_zstr_inplace<1u>("X");
   cat::verify(char_zstr == 'X');
}

$test(str_inplace_swap) {
   cat::str_inplace swap_left = "abc";
   cat::str_inplace swap_right = "xyz";
   cat::swap(swap_left, swap_right);
   cat::verify(swap_left == "xyz");
   cat::verify(swap_right == "abc");
}

$test(str_inplace_flags_and_capacity) {
   static_assert(sizeof(cat::str_inplace_fixed<8u>) == 8u);
   static_assert(sizeof(cat::zstr_inplace_fixed<8u>) == 9u);

   cat::str_inplace<8u> string;
   cat::verify(string.size() == 0u);
   cat::verify(string.capacity() == 8u);
   string.append("cat").verify();
   string.try_push_back('s').verify();
   cat::verify(string.size() == 4u);
   cat::verify(string.append("12345").is_empty());

   cat::str_inplace<8u> range_string;
   range_string.try_append_range(cat::str_view(string)).verify();
   cat::verify(range_string == "cats");

   cat::zstr_inplace<4u> terminated;
   terminated.append("cat").verify();
   cat::verify(terminated.size() == 3u);
   cat::verify(terminated.capacity() == 4u);
   cat::verify(terminated.c_str()[terminated.size()] == '\0');
   terminated.fill('x');
   cat::verify(terminated == "xxx");
   cat::verify(terminated.c_str()[terminated.size()] == '\0');

   cat::zstr_inplace_fixed<64u> abi_buffer;
   cat::verify(abi_buffer.size() == 64u);
   cat::verify(abi_buffer.capacity() == 64u);
   cat::verify(abi_buffer.c_str()[64u] == '\0');
   abi_buffer.fill('x');
   cat::verify(abi_buffer.front() == 'x');
   cat::verify(abi_buffer.back() == 'x');
   cat::verify(abi_buffer.c_str()[64u] == '\0');

   auto filled = cat::make_str_inplace_filled<4u>(3u, 'a').verify();
   auto zfilled = cat::make_zstr_inplace_filled<4u>(3u, 'b').verify();
   auto fixed_filled = cat::make_str_inplace_fixed_filled<3u>('c');
   auto zfixed_filled = cat::make_zstr_inplace_fixed_filled<3u>('d');
   cat::verify(filled == "aaa");
   cat::verify(zfilled == "bbb");
   cat::verify(fixed_filled == "ccc");
   cat::verify(zfixed_filled == "ddd");
}

$test(str_inplace_wide_char) {
   constexpr cat::wstr_inplace left = L"Hello, ";
   constexpr cat::wstr_inplace right = L"world!";
   constexpr auto joined = left + right;
   constexpr auto wrapped = L'[' + cat::wstr_inplace(L"cat") + L"]";

   static_assert(joined == L"Hello, world!");
   static_assert(wrapped == L"[cat]");
   static_assert(cat::wstr_view(cat::wstr_inplace(L"cat")) == L"cat");

   cat::wstr_inplace<8u> string;
   string.append(L"cat").verify();
   string.try_push_back(L's').verify();
   cat::verify(string == L"cats");
   cat::verify(string.size() == 4u);
   cat::verify(string.capacity() == 8u);
   string.fill(L'x');
   cat::verify(string == L"xxxx");

   cat::wzstr_inplace<4u> terminated;
   terminated.append(L"cat").verify();
   cat::verify(terminated == L"cat");
   cat::verify(terminated.c_str()[terminated.size()] == L'\0');

   cat::wstr_inplace swap_left = L"abc";
   cat::wstr_inplace swap_right = L"xyz";
   cat::swap(swap_left, swap_right);
   cat::verify(swap_left == L"xyz");
   cat::verify(swap_right == L"abc");

   auto filled = cat::make_wstr_inplace_filled<4u>(3u, L'a').verify();
   auto zfilled = cat::make_wzstr_inplace_filled<4u>(3u, L'b').verify();
   auto fixed_filled = cat::make_wstr_inplace_fixed_filled<3u>(L'c');
   auto zfixed_filled = cat::make_wzstr_inplace_fixed_filled<3u>(L'd');
   cat::verify(filled == L"aaa");
   cat::verify(zfilled == L"bbb");
   cat::verify(fixed_filled == L"ccc");
   cat::verify(zfixed_filled == L"ddd");
}
