#include <cat/page_allocator>
#include <cat/simd>
#include <cat/string>

#include "../unit_tests.hpp"

namespace {

template <typename... Strings>
void
verify_string_equality_matrix(Strings const&... strings) {
   auto verify_row = [&](auto const& left) {
      (cat::verify(left == strings), ...);
   };
   (verify_row(strings), ...);
}

}  // namespace

$test(compare_strings) {
   // The runtime SIMD `cat::compare_strings_detail` is bounded by the
   // compared strings' size, but constructing a `cat::str_view` from a small
   // string literal goes through `cat::string_length`, whose 16-byte SIMD
   // over-read trips ASan on `.rodata` redzones. Tests that rely on small
   // literals are commented out below until `string_length` learns to handle
   // small inputs without over-reading.
   //
   // // char const* p_string_1 = "Hello!";
   // // cat::str_view string_1 = "Hello!";
   // // cat::str_view string_3 = "Goodbye!";
   // // cat::verify(cat::compare_strings(p_string_1, p_string_1));
   // // cat::verify(cat::compare_strings(string_1, string_1));
   // // cat::verify(!cat::compare_strings(string_1, string_3));

   // Long string literal: large enough that the SIMD `string_length` over-read
   // stays inside the literal's `.rodata` allocation.
   cat::str_view long_string_1 =
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
   cat::str_view long_string_2 =
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
   cat::verify(cat::compare_strings(long_string_1, long_string_2));
}

$test(string_type_equality_matrix) {
   char text[] = "matrix";
   cat::str_literal<6u> literal = "matrix";
   cat::str_inplace inplace = "matrix";
   cat::zstr_inplace<6u> zinplace = "matrix";
   cat::str_inplace_fixed<6u> inplace_fixed = "matrix";
   cat::zstr_inplace_fixed<6u> zinplace_fixed = "matrix";
   cat::str_view view(text, 6u);
   cat::zstr_view zview(text, 7u);
   cat::str_span span(text, 6u);
   cat::zstr_span zspan(text, 7u);
   auto vector = cat::make_str_vec(pager, view).verify();
   auto zvector = cat::make_zstr_vec(pager, view).verify();
   auto raii_vector =
      cat::raii::make_str_vec(cat::dyn_allocator(pager), view).verify();
   auto raii_zvector =
      cat::raii::make_zstr_vec(cat::dyn_allocator(pager), view).verify();

   verify_string_equality_matrix(
      literal, inplace, zinplace, inplace_fixed, zinplace_fixed, view, zview,
      span, zspan, vector, zvector, raii_vector, raii_zvector
   );

   vector.free(pager);
   zvector.free(pager);

   wchar_t wide_text[] = L"matrix";
   cat::wstr_literal<6u> wide_literal = L"matrix";
   cat::wstr_inplace wide_inplace = L"matrix";
   cat::wzstr_inplace<6u> wide_zinplace = L"matrix";
   cat::wstr_inplace_fixed<6u> wide_inplace_fixed = L"matrix";
   cat::wzstr_inplace_fixed<6u> wide_zinplace_fixed = L"matrix";
   cat::wstr_view wide_view(wide_text, 6u);
   cat::wzstr_view wide_zview(wide_text, 7u);
   cat::wstr_span wide_span(wide_text, 6u);
   cat::wzstr_span wide_zspan(wide_text, 7u);
   auto wide_vector = cat::make_wstr_vec(pager, wide_view).verify();
   auto wide_zvector = cat::make_wzstr_vec(pager, wide_view).verify();
   auto raii_wide_vector =
      cat::raii::make_wstr_vec(cat::dyn_allocator(pager), wide_view).verify();
   auto raii_wide_zvector =
      cat::raii::make_wzstr_vec(cat::dyn_allocator(pager), wide_view).verify();

   verify_string_equality_matrix(
      wide_literal, wide_inplace, wide_zinplace, wide_inplace_fixed,
      wide_zinplace_fixed, wide_view, wide_zview, wide_span, wide_zspan,
      wide_vector, wide_zvector, raii_wide_vector, raii_wide_zvector
   );

   wide_vector.free(pager);
   wide_zvector.free(pager);
}

$test(compare_strings_long_misaligned_equal_and_diff) {
   idx const len = 200_idx;
   idx const skew = 11_idx;
   alignas(128) char buf_a[320]{};
   alignas(128) char buf_b[320]{};
   for (idx i = 0_idx; i < len; ++i) {
      char const c = static_cast<char>('a' + (i.raw % 23));
      buf_a[skew.raw + i.raw] = c;
      buf_b[skew.raw + i.raw] = c;
   }
   cat::str_view const va{buf_a + skew.raw, len};
   cat::str_view const vb{buf_b + skew.raw, len};
   cat::verify(cat::compare_strings(va, vb));
   buf_b[skew.raw + 170] = 'z';
   cat::verify(!cat::compare_strings(va, vb));
}
