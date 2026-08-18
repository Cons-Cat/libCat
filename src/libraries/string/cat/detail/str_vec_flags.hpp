// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/container>

#include "./str_flags.hpp"

namespace cat {

struct str_vec_flags {
   str_flags const str;
   vec_flags const vec = {};

   constexpr str_vec_flags() = default;

   constexpr str_vec_flags(str_flags flags) : str(flags) {
   }

   constexpr str_vec_flags(vec_flags flags) : vec(flags) {
   }

   [[nodiscard]]
   friend constexpr auto
   operator|(str_vec_flags left, str_vec_flags right) -> str_vec_flags {
      return str_vec_flags{left.str | right.str, left.vec | right.vec};
   }

   [[nodiscard]]
   friend constexpr auto
   operator|(str_flags left, str_vec_flags right) -> str_vec_flags {
      return str_vec_flags{left | right.str, right.vec};
   }

   [[nodiscard]]
   friend constexpr auto
   operator|(str_vec_flags left, str_flags right) -> str_vec_flags {
      return str_vec_flags{left.str | right, left.vec};
   }

   friend constexpr auto
   operator|(str_flags string_flags, vec_flags vector_flags) -> str_vec_flags;

 private:
   constexpr str_vec_flags(str_flags string_flags, vec_flags vector_flags)
       : str(string_flags), vec(vector_flags) {
   }
};

[[nodiscard]]
constexpr auto
operator|(str_flags string_flags, vec_flags vector_flags) -> str_vec_flags {
   return str_vec_flags{string_flags, vector_flags};
}

namespace detail {
inline constexpr bool is_literal_encoding_utf8 =
   __builtin_strcmp(__clang_literal_encoding__, "UTF-8") == 0;
inline constexpr bool is_wide_literal_encoding_utf16 =
   __builtin_strcmp(__clang_wide_literal_encoding__, "UTF-16") == 0;
inline constexpr bool is_wide_literal_encoding_utf32 =
   __builtin_strcmp(__clang_wide_literal_encoding__, "UTF-32") == 0;
}  // namespace detail

template <typename CharT>
concept is_string_char =
   is_same<remove_const<CharT>, char> || is_same<remove_const<CharT>, wchar_t>
   || is_same<remove_const<CharT>, char8_t>
   || is_same<remove_const<CharT>, char16_t>
   || is_same<remove_const<CharT>, char32_t>;

template <typename CharT>
concept is_char_utf8_interconvertible =
   is_same<remove_const<CharT>, char8_t>
   || (detail::is_literal_encoding_utf8 && is_same<remove_const<CharT>, char>);

namespace detail {
template <typename Left, typename Right>
consteval auto
encoding_compatible_string_char_impl() -> bool {
   using left_char = remove_const<Left>;
   using right_char = remove_const<Right>;
   if constexpr (sizeof(left_char) != sizeof(right_char)) {
      return false;
   } else {
      constexpr bool same_char = is_same<left_char, right_char>;
      constexpr bool same_utf8 = is_char_utf8_interconvertible<left_char>
                                 && is_char_utf8_interconvertible<right_char>;
      constexpr bool wide_and_utf16 =
         (is_same<left_char, wchar_t> && is_same<right_char, char16_t>)
         || (is_same<left_char, char16_t> && is_same<right_char, wchar_t>);
      constexpr bool wide_and_utf32 =
         (is_same<left_char, wchar_t> && is_same<right_char, char32_t>)
         || (is_same<left_char, char32_t> && is_same<right_char, wchar_t>);
      return same_char || same_utf8
             || (wide_and_utf16 && is_wide_literal_encoding_utf16)
             || (wide_and_utf32 && is_wide_literal_encoding_utf32);
   }
}

template <typename Left, typename Right>
consteval auto
encoding_compatible_char_impl() -> bool {
   if constexpr (is_string_char<Left> && is_string_char<Right>) {
      return encoding_compatible_string_char_impl<Left, Right>();
   } else {
      return false;
   }
}
}  // namespace detail

template <typename Left, typename Right>
concept encoding_compatible_char =
   detail::encoding_compatible_char_impl<Left, Right>();

}  // namespace cat
