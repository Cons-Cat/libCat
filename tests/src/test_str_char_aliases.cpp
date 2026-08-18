#include <cat/linear_allocator>
#include <cat/page_allocator>
#include <cat/string>

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

$test(str_span_character_aliases) {
   static_assert(cat::is_same<cat::str_view, cat::basic_str_span<char const>>);
   static_assert(
      cat::is_same<cat::u8str_view, cat::basic_str_span<char8_t const>>
   );
   static_assert(
      cat::is_same<cat::u16str_view, cat::basic_str_span<char16_t const>>
   );
   static_assert(
      cat::is_same<cat::u32str_view, cat::basic_str_span<char32_t const>>
   );
   static_assert(
      cat::is_same<
         cat::zu8str_span,
         cat::basic_str_span<char8_t, cat::str_flags::null_terminated>>
   );

   // `भारत` is four Devanagari code points. UTF-8 stores three bytes each.
   constexpr cat::u8str_view utf8 = u8"भारत";
   constexpr cat::u16str_view utf16 = u"भारत";
   constexpr cat::u32str_view utf32 = U"भारत";
   static_assert(utf8.size() == 12u);
   static_assert(utf16.size() == 4u);
   static_assert(utf32.size() == 4u);
   static_assert(utf8[0] == u8'\xE0');
   static_assert(utf16[0] == u'भ');
   static_assert(utf32[2] == U'र');

   static_assert(cat::detail::is_literal_encoding_utf8);
   static_assert(cat::encoding_compatible_char<char, char8_t>);
   static_assert(cat::is_constructible<cat::str_view, cat::u8str_view>);
   static_assert(!cat::is_constructible<cat::u8str_view, char const*>);
   static_assert(!cat::is_constructible<cat::str_view, char8_t const*>);
   static_assert(!cat::is_constructible<cat::u16str_view, char const*>);

   static_assert(cat::detail::is_wide_literal_encoding_utf32);
   static_assert(cat::encoding_compatible_char<wchar_t, char32_t>);
   static_assert(!cat::encoding_compatible_char<wchar_t, char16_t>);
   static_assert(!cat::is_constructible<cat::u32str_view, wchar_t const*>);
   static_assert(!cat::is_constructible<cat::wstr_view, char32_t const*>);
   static_assert(!cat::is_constructible<cat::u16str_view, wchar_t const*>);
}

$test(str_inplace_character_aliases) {
   static_assert(
      cat::is_same<cat::u8str_inplace<8u>, cat::basic_str_inplace<char8_t, 8u>>
   );
   static_assert(
      cat::is_same<
         cat::zu16str_inplace_fixed<4u>, cat::basic_str_inplace<
                                            char16_t, 4u,
                                            cat::str_flags::null_terminated
                                               | cat::vec_flags::fixed_size
                                               | cat::str_vec_flags{}>>
   );

   constexpr cat::u8str_inplace utf8 = u8"भारत";
   constexpr cat::u16str_inplace utf16 = u"भारत";
   constexpr cat::u32str_inplace utf32 = U"भारत";
   static_assert(utf8 == u8"भारत");
   static_assert(utf16 == u"भारत");
   static_assert(utf32 == U"भारत");
   static_assert(cat::u8str_view(utf8) == cat::u8str_view(u8"भारत"));

   constexpr cat::u8str_inplace<12u> utf8_from_char = "भारत";
   constexpr cat::u32str_inplace<4u> utf32_from_wide = L"भारत";
   static_assert(utf8_from_char == u8"भारत");
   static_assert(utf32_from_wide == U"भारत");
   static_assert(
      !cat::is_constructible<cat::u16str_inplace<4u>, wchar_t const(&)[5]>
   );
}

$test(str_vec_character_aliases) {
   static_assert(cat::is_same<cat::u8str_vec, cat::basic_str_vec<char8_t>>);
   static_assert(cat::is_same<
                 cat::zu16str_vec,
                 cat::basic_str_vec<
                    char16_t, cat::str_flags::null_terminated
                                 | cat::vec_flags::pointer_size_layout>>);
   static_assert(cat::is_same<
                 cat::small_u32str_vec<8u>,
                 cat::basic_str_vec<
                    char32_t, cat::vec_flags::inline_storage(8u)
                                 | cat::vec_flags::pointer_size_layout>>);
   static_assert(
      cat::is_same<
         cat::raii::u8str_vec<>,
         cat::raii::basic_str_vec<
            char8_t, cat::vec_flags::pointer_size_layout, cat::dyn_allocator>>
   );
   static_assert(cat::is_same<
                 cat::raii::zu32str_vec_fixed<>,
                 cat::raii::basic_str_vec<
                    char32_t,
                    cat::str_flags::null_terminated
                       | cat::vec_flags::fixed_size
                       | cat::vec_flags::pointer_size_layout,
                    cat::dyn_allocator>>);

   linear_arena arena;

   cat::u8str_vec utf8;
   utf8.append(arena.alloc, "भारत").verify();
   cat::verify(utf8.view() == cat::u8str_view(u8"भारत"));
   utf8.free(arena.alloc);

   cat::u16str_vec utf16;
   utf16.append(arena.alloc, cat::u16str_view(u"भारत")).verify();
   cat::verify(utf16.view() == cat::u16str_view(u"भारत"));
   utf16.free(arena.alloc);

   cat::raii::u32str_vec<cat::linear_allocator> managed(arena.alloc);
   managed.append(cat::u32str_view(U"भारत")).verify();
   cat::verify(managed.view() == cat::u32str_view(U"भारत"));

   cat::raii::zu8str_vec<cat::linear_allocator> zmanaged(arena.alloc);
   zmanaged.append(cat::u8str_view(u8"भारत")).verify();
   cat::verify(zmanaged.size() == 12u);
   cat::verify(zmanaged.data()[zmanaged.size()] == u8'\0');
   cat::verify(cat::u8str_view(zmanaged) == cat::u8str_view(u8"भारत"));
}
