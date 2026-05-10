#include <cat/string>

#include "../unit_tests.hpp"

// `str_inplace` and `zstr_inplace` constructors from string literals are
// `consteval`, so the length is known at compile time and the SIMD
// `cat::string_length` is never invoked.

$test(str_inplace_construct_and_concat) {
   constexpr cat::str_inplace const_string_3 = "Hello, ";
   constexpr cat::str_inplace const_string_4 = "world!";

   cat::verify(!const_string_3.at(10).has_value());

   // TODO: Make this `constexpr`.
   cat::str_inplace hello_world = (const_string_3 + const_string_4);
   constexpr cat::str_inplace const_hello_world =
      (const_string_3 + const_string_4);

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

   cat::verify(hello_world == const_hello_world);
   cat::verify(const_hello_world_7 == "Hello, world!");
}

$test(str_inplace_compare_to_char) {
   cat::str_inplace char_str = "X";
   cat::verify(char_str == 'X');

   cat::zstr_inplace char_zstr = cat::make_zstr_inplace<2u>("X");
   cat::verify(char_zstr == 'X');
}

$test(str_inplace_swap) {
   cat::str_inplace swap_left = "abc";
   cat::str_inplace swap_right = "xyz";
   cat::swap(swap_left, swap_right);
   cat::verify(swap_left == "xyz");
   cat::verify(swap_right == "abc");
}
