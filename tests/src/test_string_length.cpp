#include <cat/string>
#include <cat/utility>

#include "unit_tests.hpp"

TEST(test_string_length) {
   char const* p_string_1 = "Hello!";
   char const* const p_string_2 = "Hello!";

   char const* const p_string_3 = "Hello!";
   char const* p_string_4 = "Hello!";

   char const* const p_string_5 = "Hello!";
   char const* const p_string_6 = "Hello!";

   char const* p_string_7 = "/tmp/temp.sock";

   cat::iword len_1 = cat::string_length(p_string_1);
   cat::iword len_2 = cat::string_length(p_string_2);
   cat::iword len_3 = cat::string_length(p_string_3);
   cat::iword len_4 = cat::string_length(p_string_4);
   cat::iword len_5 = cat::string_length(p_string_5);
   cat::iword len_6 = cat::string_length(p_string_6);
   cat::iword len_7 = cat::string_length(p_string_7);

   cat::verify(len_1 == len_2);
   cat::verify(len_1 == 6);
   cat::verify(len_3 == len_4);
   cat::verify(len_5 == len_6);
   cat::verify(len_7 == 14);

   // Test `string`s.
   cat::str_view string_1 = p_string_1;
   cat::verify(string_1.size() == len_1);
   cat::verify(string_1.subspan(1, 4).size() == 3);
   cat::verify(string_1.first(4).size() == 4);
   cat::verify(string_1.last(3).size() == 3);
   cat::verify(cat::str_view("Hello!").size() == len_1);

   auto inplace = cat::make_str_inplace<10u>("Hello");
   cat::verify(inplace.size() == 10);  // "Hello\0\0\0\0\0"

   auto inplace_2 = cat::make_str_inplace<5u>("Hello");
   cat::verify(inplace_2.size() == 5);

   auto inplace_3 = cat::str_inplace("Hello");
   cat::verify(inplace_3.size() == 5);

   auto strv = cat::str_view("Hello");
   cat::verify(strv.size() == 5);

   cat::zstr_view zstr("Hello");
   cat::verify(zstr.size() == 6);
   zstr = p_string_6;
   cat::verify(zstr.size() == 7);

   cat::str_view str_over_zstr = zstr;
   cat::verify(str_over_zstr.size() == 6);
   str_over_zstr = zstr;
   cat::verify(str_over_zstr.size() == 6);

   cat::page_allocator pager;
   cat::zstr_span mut_zstr = pager.calloc_multi<char>(6).verify();
   defer {
      pager.free(mut_zstr);
   };
   cat::verify(mut_zstr.size() == 6);
   cat::verify(cat::zstr_view(mut_zstr).size() == 6);

   cat::verify(cat::str_span(mut_zstr).size() == 5);
   cat::verify(cat::str_view(mut_zstr).size() == 5);
   cat::verify(cat::str_view(cat::zstr_view(mut_zstr)).size() == 5);

   cat::zstr_inplace<10u> inplace_z = cat::make_zstr_inplace<10u>("Hello");
   cat::verify(inplace_z.size() == 10);  // "Hello\0\0\0\0\0"
   cat::verify(cat::str_span(inplace_z).size() == 9);
   cat::verify(cat::str_view(inplace_z).size() == 9);

   cat::verify(cat::zstr_span(inplace_z).size() == 10);
   cat::verify(cat::zstr_view(inplace_z).size() == 10);

   auto inplace_z_2 = cat::make_zstr_inplace<6u>("Hello");
   cat::verify(inplace_z_2.size() == 6);  // "Hello\0"

   auto inplace_z_3 = inplace_z_2 + inplace_z_2;
   cat::verify(inplace_z_3.size() == 11);  // "HelloHello\0"
}
