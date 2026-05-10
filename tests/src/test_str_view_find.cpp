#include <cat/simd>
#include <cat/string>

#include "../unit_tests.hpp"

// `cat::basic_str_span::find` does its own bounded SIMD scan (`size`-bounded,
// no over-read), so its tests can use small inputs as long as the underlying
// `str_view` is constructed without going through SIMD `string_length`. The
// fixtures below feed `find` a `str_view{ptr, len}` over a stack array.

$test(str_view_find_past_first_simd_chunk) {
   idx const lanes = cat::char1x16::size();

   char buffer[128]{};
   for (idx i = 0u; i < lanes; ++i) {
      buffer[i.raw] = 'a';
   }
   idx const past_first_chunk = lanes + 7u;
   buffer[past_first_chunk.raw] = 'Z';

   cat::str_view const haystack_past_chunk{buffer, past_first_chunk + 1u};
   cat::verify(haystack_past_chunk.find('Z').value() == past_first_chunk);
   cat::verify(!haystack_past_chunk.find('q').has_value());

   char buffer2[128]{};
   buffer2[4] = 'm';
   cat::str_view const haystack_first_chunk(buffer2, 48u);
   cat::verify(haystack_first_chunk.find('m').value() == 4);
}

$test(str_view_over_str_inplace_find_single_chars) {
   constexpr cat::str_inplace<7> hello = "Hello, ";
   // Conversion `str_inplace -> str_view` uses the .data()/.size() ctor, not
   // SIMD `string_length`, so it is safe under ASan.
   cat::str_view const hello_view = hello;
   idx const h = hello_view.find('H').value();
   idx const e = hello_view.find('e').value();
   idx const l = hello_view.find('l').value();
   idx const o = hello_view.find('o').value();
   cat::verify(h == 0);
   cat::verify(e == 1);
   cat::verify(l == 2);
   cat::verify(o == 4);
}
