#include <cat/string>

#include "../unit_tests.hpp"

TEST(test_compare_strings) {
   char const* p_string_1 = "Hello!";
   char const* const p_string_2 = "Hello!";

   cat::str_view string_1 = "Hello!";
   cat::str_view const string_2 = "Hello!";
   cat::str_view string_3 = "Goodbye!";

   cat::str_view long_string_1 =
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
   cat::str_view long_string_2 =
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

   // Test a succesful string pointer case.
   cat::verify(cat::compare_strings(p_string_1, p_string_2));

   // Test a succesful string case.
   cat::verify(cat::compare_strings(string_1, string_2));

   // Test a succesful large string case.
   cat::verify(cat::compare_strings(long_string_1, long_string_2));

   // Test a failure case.
   cat::verify(!cat::compare_strings(string_1, string_3));

   [[maybe_unused]]
   cat::str_view const_string_1 = "Hello, ";
   [[maybe_unused]]
   constexpr cat::str_view const_string_2 = "world!";

   // Fixed length strings.
   constexpr cat::str_inplace const_string_3 = "Hello, ";
   constexpr cat::str_inplace const_string_4 = "world!";

   // Test collection operations.
   auto _ = const_string_1[1];
   cat::verify(!const_string_3.at(10).has_value());

   // TODO: Make this `constexpr`.
   cat::str_inplace hello_world = (const_string_3 + const_string_4);
   constexpr cat::str_inplace const_hello_world =
      (const_string_3 + const_string_4);

   constexpr cat::str_view const_hello_world_2 = "Hello, world!";
   constexpr cat::str_view const_hello_world_3 = const_hello_world_2;
   constexpr cat::str_view const_hello_world_4 = const_hello_world_3;
   constexpr cat::str_inplace<13> const_hello_world_5 =
      cat::str_inplace("Hello, ") + "world!";
   constexpr cat::zstr_inplace<14> const_hello_world_6 =
      cat::make_zstr_inplace<8>("Hello, ") + "world!";
   constexpr cat::str_inplace const_hello_world_7 =
      "Hello, " + cat::str_inplace("world!");
   cat::zstr_inplace<14> const_hello_world_8 =
      "Hello, " + cat::make_zstr_inplace<7>("world!");
   const_hello_world_8 = "Hello, world!";
   static_assert(const_hello_world_5 == "Hello, world!");
   static_assert(const_hello_world_6 == "Hello, world!");

   cat::verify(cat::compare_strings(hello_world, "Hello, world!"));
   cat::verify(cat::compare_strings(const_hello_world, "Hello, world!"));
   cat::verify(cat::compare_strings(const_hello_world_2, "Hello, world!"));
   cat::verify(cat::compare_strings(const_hello_world_3, "Hello, world!"));

   // TODO: Bind a `str_view` to `zstr`  containers.
   // TODO: Pass `zstr` containers into `cat::compare_strings()`.

   iword const h = const_string_1.find('H').value();
   iword const e = const_string_1.find('e').value();
   iword const l = const_string_1.find('l').value();
   iword const o = const_string_1.find('o').value();
   cat::verify(h == 0);
   cat::verify(e == 1);
   cat::verify(l == 2);
   cat::verify(o == 4);

   // Compare single-character in-place strings to a `char`.
   cat::str_inplace char_str = "X";
   cat::verify(char_str == 'X');

   cat::zstr_inplace char_zstr = cat::make_zstr_inplace<2u>("X");
   cat::verify(char_zstr == 'X');
}
