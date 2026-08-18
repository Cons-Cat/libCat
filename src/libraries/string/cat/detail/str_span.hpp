// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// `cat::basic_str_span` is a non-owning string container over a contiguous
// sequence of characters. It is inspired by `std::basic_string_view` and is
// analogous to `cat::span`. It is parameterized by a character type and a
// `cat::str_flags` describing whether the view is null-terminated.
//
// Convenience type aliases are provided:
//
//  `cat::str_view`, a `basic_str_span<char const>`.
//  `cat::str_span`, a `basic_str_span<char>`.
//  `cat::zstr_view`, a `basic_str_span<char const,
//  str_flags::null_terminated>`. `cat::zstr_span`, a `basic_str_span<char,
//  str_flags::null_terminated>`. `cat::u8str_view` / `u8str_span` /
//  `zu8str_view` / `zu8str_span`. `cat::u16str_view` / `u16str_span` /
//  `zu16str_view` / `zu16str_span`. `cat::u32str_view` / `u32str_span` /
//  `zu32str_view` / `zu32str_span`. `cat::wstr_view`, a `basic_str_span<wchar_t
//  const>`. `cat::wstr_span`, a `basic_str_span<wchar_t>`. `cat::wzstr_view`, a
//  null-terminated `basic_str_span<wchar_t const>`. `cat::wzstr_span`, a
//  null-terminated `basic_str_span<wchar_t>`.
//
// TODO: `str_span` needs `fixed_extent` like `span`.

#include <cat/detail/simd_impl.hpp>

#include <cat/maybe>
#include <cat/simd>
#include <cat/span>
#include <cat/utility>

#include "./str_vec_flags.hpp"
#include "./str_inplace.hpp"

namespace cat {

template <typename CharT>
[[clang::no_builtin("strlen")]]
constexpr auto
string_length(CharT const* _Nonnull p_string) -> idx {
   idx result;
   while (true) {
      if (p_string[result.raw] == CharT{'\0'}) {
         return result;
      }
      ++result;
   }
}

template <typename CharT, str_flags flags = {}>
class basic_str_span;

inline namespace manual {
template <typename CharT, str_vec_flags flags>
class basic_str_vec;
}  // namespace manual

using str_view = basic_str_span<char const>;
using str_span = basic_str_span<char>;
using zstr_view = basic_str_span<char const, str_flags::null_terminated>;
using zstr_span = basic_str_span<char, str_flags::null_terminated>;

using u8str_view = basic_str_span<char8_t const>;
using u8str_span = basic_str_span<char8_t>;
using zu8str_view = basic_str_span<char8_t const, str_flags::null_terminated>;
using zu8str_span = basic_str_span<char8_t, str_flags::null_terminated>;

using u16str_view = basic_str_span<char16_t const>;
using u16str_span = basic_str_span<char16_t>;
using zu16str_view = basic_str_span<char16_t const, str_flags::null_terminated>;
using zu16str_span = basic_str_span<char16_t, str_flags::null_terminated>;

using u32str_view = basic_str_span<char32_t const>;
using u32str_span = basic_str_span<char32_t>;
using zu32str_view = basic_str_span<char32_t const, str_flags::null_terminated>;
using zu32str_span = basic_str_span<char32_t, str_flags::null_terminated>;

using wstr_view = basic_str_span<wchar_t const>;
using wstr_span = basic_str_span<wchar_t>;
using wzstr_view = basic_str_span<wchar_t const, str_flags::null_terminated>;
using wzstr_span = basic_str_span<wchar_t, str_flags::null_terminated>;

template <typename CharT>
[[nodiscard]]
constexpr auto
compare_strings_scalar(
   basic_str_span<CharT const> string_1, basic_str_span<CharT const> string_2
) -> bool {
   if (string_1.size() != string_2.size()) {
      return false;
   }

   for (idx i = 0u; i < string_1.size(); ++i) {
      if (string_1[i] != string_2[i]) {
         return false;
      }
   }

   return true;
}

template <typename CharT>
[[nodiscard]]
constexpr auto
compare_strings(
   basic_str_span<CharT const> string_1, basic_str_span<CharT const> string_2
) -> bool {
   return compare_strings_scalar(string_1, string_2);
}

[[nodiscard]]
constexpr auto
compare_strings(str_view string_1, str_view string_2) -> bool;

template <typename CharT, str_flags flags>
class
   [[clang::preferred_name(str_view), clang::preferred_name(str_span),
     clang::preferred_name(zstr_view), clang::preferred_name(zstr_span),
     clang::preferred_name(u8str_view), clang::preferred_name(u8str_span),
     clang::preferred_name(zu8str_view), clang::preferred_name(zu8str_span),
     clang::preferred_name(u16str_view), clang::preferred_name(u16str_span),
     clang::preferred_name(zu16str_view), clang::preferred_name(zu16str_span),
     clang::preferred_name(u32str_view), clang::preferred_name(u32str_span),
     clang::preferred_name(zu32str_view), clang::preferred_name(zu32str_span),
     clang::preferred_name(wstr_view), clang::preferred_name(wstr_span),
     clang::preferred_name(wzstr_view), clang::preferred_name(wzstr_span),
     gsl::Pointer(CharT)]]
   basic_str_span : public span<CharT> {
 public:
   constexpr basic_str_span() : span<CharT>(nullptr) {
   }

   constexpr basic_str_span(basic_str_span const& string) = default;

   constexpr basic_str_span(CharT* _Nonnull p_string, idx in_length)
       : span<CharT>(p_string, in_length) {
   }

   template <typename T>
      requires(
         is_same<CharT, char const> && !flags.is_null_terminated
         && is_same<T, char8_t const>
      )
   constexpr basic_str_span(
      basic_str_span<T> string [[clang::lifetimebound]]
   ) {
      this->m_p_data = reinterpret_cast<char const*>(string.data());
      this->m_size = string.size();
   }

   // This weird template deduces lower than the string literal constructor.
   template <is_pointer T>
      requires(
         is_same<remove_const<CharT>, remove_const<remove_pointer<T>>>
         && (!is_const<remove_pointer<T>> || is_const<CharT>)
      )
   constexpr basic_str_span(
      T _Nonnull p_string [[clang::lifetimebound]]
   )
       : span<CharT>(
            p_string, string_length(p_string) + flags.is_null_terminated
         ) {
   }

   template <is_pointer T>
      requires(is_string_char<remove_pointer<T>>
               && !is_same<
                  remove_const<CharT>, remove_const<remove_pointer<T>>>)
   constexpr basic_str_span(T) =
      delete ("Cannot construct a `str_span` over a different character "
              "encoding! Copy into an owning string instead.");

   template <is_pointer T>
      requires(is_same<remove_const<CharT>, remove_const<remove_pointer<T>>>
               && is_const<remove_pointer<T>> && !is_const<CharT>)
   constexpr basic_str_span(T) =
      delete ("Cannot construct a mutable `str_span` over a `const` string!");

   // Zero-overhead string literal constructor.
   template <idx other_length>
   consteval basic_str_span(CharT (&string)[other_length]) {
      // Verify that the final character is null terminator. This is necessary
      // if `constexpr char` arrays are passed in rather than string literals.
      [[assume(string[other_length.raw - 1uz] == remove_const<CharT>{'\0'})]];
      this->m_p_data = string;
      // Subtract 1 to length to ignore a null terminator. For `zstr_span` the
      // assume forces a trailing null so `other_length` is at least one and
      // subtracting one cannot wrap. For `str_span` the subtrahend is zero.
      this->m_size = idx(other_length - (flags.is_null_terminated ? 0u : 1u));
   }

   template <idx other_length>
      requires(!is_const<CharT>)
   consteval basic_str_span(CharT const (&string)[other_length]) =
      delete ("Cannot construct a mutable `str_span` over a `char const*` "
              "literal! Consider a `str_view` instead.");

   // Make a `str_span` over a `str_inplace`.
   template <idx other_capacity, str_vec_flags other_flags>
      requires(flags.is_null_terminated == other_flags.str.is_null_terminated)
   constexpr basic_str_span(
      basic_str_inplace<CharT, other_capacity, other_flags>& other_string
      [[clang::lifetimebound]]
   ) {
      this->m_p_data = other_string.data();
      this->m_size =
         other_string.size() + static_cast<unsigned>(flags.is_null_terminated);
   }

   // Make a `str_view` over a `str_inplace`.
   template <idx other_capacity, str_vec_flags other_flags>
      requires(
         is_const<CharT>
         && flags.is_null_terminated == other_flags.str.is_null_terminated
      )
   constexpr basic_str_span(
      basic_str_inplace<remove_const<CharT>, other_capacity, other_flags> const&
         other_string [[clang::lifetimebound]]
   ) {
      this->m_p_data = other_string.data();
      this->m_size =
         other_string.size() + static_cast<unsigned>(flags.is_null_terminated);
   }

   // Make a `str_span` over a `zstr_inplace`.
   template <typename T, idx other_capacity, str_vec_flags other_flags>
      requires(
         !flags.is_null_terminated && other_flags.str.is_null_terminated
         && is_same<CharT, T>
      )
   constexpr basic_str_span(
      basic_str_inplace<T, other_capacity, other_flags>& other_string
      [[clang::lifetimebound]]
   ) {
      this->m_p_data = other_string.data();
      this->m_size = other_string.size();
   }

   // Make a `str_view` over a `zstr_inplace`.
   template <typename T, idx other_capacity, str_vec_flags other_flags>
      requires(
         is_const<CharT> && !flags.is_null_terminated
         && other_flags.str.is_null_terminated
         && is_same<remove_const<CharT>, T>
      )
   constexpr basic_str_span(
      basic_str_inplace<T, other_capacity, other_flags> const& other_string
      [[clang::lifetimebound]]
   ) {
      this->m_p_data = other_string.data();
      this->m_size = other_string.size();
   }

   // Make a `str_span` over a `zstr_inplace`.
   template <idx other_capacity, str_vec_flags other_flags>
      requires(flags.is_null_terminated
               && !is_const<CharT> && !other_flags.str.is_null_terminated)
   constexpr basic_str_span(
      basic_str_inplace<CharT, other_capacity, other_flags> const&
   ) = delete ("Cannot bind a null-terminated `zstr_span` over a "
               "non null-terminated `str_inplace`!");

   template <str_vec_flags other_flags>
      requires(
         !is_const<CharT>
         && flags.is_null_terminated == other_flags.str.is_null_terminated
      )
   constexpr basic_str_span(
      basic_str_vec<CharT, other_flags>& other_string [[clang::lifetimebound]]
   ) {
      this->m_p_data = other_string.data();
      this->m_size =
         other_string.size() + static_cast<unsigned>(flags.is_null_terminated);
   }

   template <str_vec_flags other_flags>
      requires(
         is_const<CharT>
         && flags.is_null_terminated == other_flags.str.is_null_terminated
      )
   constexpr basic_str_span(
      basic_str_vec<remove_const<CharT>, other_flags> const& other_string
      [[clang::lifetimebound]]
   ) {
      this->m_p_data = other_string.data();
      this->m_size =
         other_string.size() + static_cast<unsigned>(flags.is_null_terminated);
   }

   template <str_vec_flags other_flags>
      requires(
         !is_const<CharT> && !flags.is_null_terminated
         && other_flags.str.is_null_terminated
      )
   constexpr basic_str_span(
      basic_str_vec<CharT, other_flags>& other_string [[clang::lifetimebound]]
   ) {
      this->m_p_data = other_string.data();
      this->m_size = other_string.size();
   }

   template <str_vec_flags other_flags>
      requires(
         is_const<CharT> && !flags.is_null_terminated
         && other_flags.str.is_null_terminated
      )
   constexpr basic_str_span(
      basic_str_vec<remove_const<CharT>, other_flags> const& other_string
      [[clang::lifetimebound]]
   ) {
      this->m_p_data = other_string.data();
      this->m_size = other_string.size();
   }

   // Promote `basic_str_span<char>` to `basic_str_span<char const>`.
   template <typename T>
      requires(is_same<CharT, add_const<T>>)
   constexpr basic_str_span(
      basic_str_span<T, flags> in_span [[clang::lifetimebound]]
   )
       : span<CharT>(in_span) {
   }

   // Promote `zstr_span` to `str_span`.
   template <typename T>
      requires(
         !is_const<CharT> && is_same<CharT, T> && !flags.is_null_terminated
      )
   constexpr basic_str_span(
      basic_str_span<T, str_flags::null_terminated> in_span
      [[clang::lifetimebound]]
   ) {
      this->m_p_data = in_span.data();
      // `zstr_span` size counts the trailing null, so it is never zero here.
      this->m_size = idx(in_span.size() - 1u);
   }

   // Promote `zstr_span<char>` to `str_span<char const>`.
   template <typename T>
      requires(is_same<CharT, add_const<T>> && !flags.is_null_terminated)
   constexpr basic_str_span(
      basic_str_span<T, str_flags::null_terminated> in_span
      [[clang::lifetimebound]]
   ) {
      this->m_p_data = in_span.data();
      // Same as the non-`const` promotion above.
      this->m_size = idx(in_span.size() - 1u);
   }

   // Prevent binding a `zstr_span` over a `str_span`.
   constexpr basic_str_span(basic_str_span<CharT> const&)
      requires(flags.is_null_terminated)
   = delete ("Cannot bind a null-terminated `zstr_span` over a "
             "non null-terminated `str_span`!");

   // Promote `span<char>` to `basic_str_span<char>`.
   constexpr basic_str_span(span<CharT> in_span [[clang::lifetimebound]])
       : span<CharT>(in_span) {
   }

   template <typename String>
      requires(
         !is_const<CharT>
         && is_same<CharT, typename remove_cvref<String>::value_type>
         && !is_same<remove_cvref<String>, basic_str_span<remove_const<CharT>>>
         && !is_same<
            remove_cvref<String>,
            basic_str_span<remove_const<CharT>, str_flags::null_terminated>>
      )
   constexpr basic_str_span(
      String& string [[clang::lifetimebound]]
   ) {
      this->m_p_data = string.data();
      this->m_size =
         string.size() + static_cast<unsigned>(flags.is_null_terminated);
      bool const source_is_null_terminated = string.is_null_terminated();
      if constexpr (flags.is_null_terminated) {
         assert(source_is_null_terminated);
      }
   }

   template <typename String>
      requires(
         is_const<CharT>
         && is_same<
            remove_const<CharT>, typename remove_cvref<String>::value_type>
         && !is_same<remove_cvref<String>, basic_str_span<remove_const<CharT>>>
         && !is_same<
            remove_cvref<String>,
            basic_str_span<remove_const<CharT>, str_flags::null_terminated>>
      )
   constexpr basic_str_span(
      String const& string [[clang::lifetimebound]]
   ) {
      this->m_p_data = string.data();
      this->m_size =
         string.size() + static_cast<unsigned>(flags.is_null_terminated);
      bool const source_is_null_terminated = string.is_null_terminated();
      if constexpr (flags.is_null_terminated) {
         assert(source_is_null_terminated);
      }
   }

   // A `string` consuming `nullptr` would cause undefined behavior.
   constexpr basic_str_span(decltype(nullptr)) = delete;

   constexpr auto
   operator=(basic_str_span const& other_string) -> basic_str_span& {
      // Omitting `[[clang::lifetimebound]]` on the RHS avoids Clang 23
      // `-Wdangling-assignment` false positives on assigns from pointers or
      // arrays whose storage still outlives this view.
      this->m_p_data = other_string.data();
      this->m_size = other_string.size();
      return *this;
   }

   [[nodiscard]]
   constexpr auto
   is_null_terminated() const -> bool {
      if constexpr (flags.is_null_terminated) {
         return true;
      } else {
         return this->m_size != 0u
                && this->m_p_data[this->m_size - 1u]
                      == remove_const<CharT>{'\0'};
      }
   }

   friend constexpr auto
   operator==(
      basic_str_span const& this_string, basic_str_span const& other_string
   ) {
      using char_type = remove_const<CharT>;
      using view_type = basic_str_span<char_type const>;
      return compare_strings(view_type(this_string), view_type(other_string));
   }

   constexpr auto
   fill(remove_const<CharT> value) -> basic_str_span&
      requires(!is_const<CharT>)
   {
      idx const content_size =
         idx(this->size() - static_cast<unsigned>(flags.is_null_terminated));
      for (idx index; index < content_size; ++index) {
         (*this)[index] = value;
      }
      return *this;
   }

   // TODO: Make these member functions `const`.

   [[nodiscard]]
   constexpr auto
   substring(idx position, idx count) -> basic_str_span<CharT> {
      // Omitting `[[clang::lifetimebound]]` avoids Clang 23
      // `-Wdangling-assignment` false positives when slicing still aliases the
      // same backing buffer.
      return {this->m_p_data + position, count};
   }

   [[nodiscard]]
   constexpr auto
   remove_prefix(idx offset) -> basic_str_span {
      // Omitting `[[clang::lifetimebound]]` avoids Clang 23
      // `-Wdangling-assignment` false positives when slicing still aliases the
      // same backing buffer.
      return {this->m_p_data + offset, idx(this->m_size - offset)};
   }

   [[nodiscard]]
   constexpr auto
   remove_suffix(idx offset) -> basic_str_span<CharT> {
      // Omitting `[[clang::lifetimebound]]` avoids Clang 23
      // `-Wdangling-assignment` false positives when slicing still aliases the
      // same backing buffer.
      return this->substring(0u, idx(this->m_size - offset));
   }

   [[nodiscard]]
   constexpr auto
   find_small(CharT character, idx position = 0u) const -> maybe<idx> {
      for (idx i = position; i < this->m_size; ++i) {
         if (this->m_p_data[i] == character) {
            return i;
         }
      }
      return nullopt;
   }

   // TODO: Optimize different length strings.
   [[nodiscard]]
   constexpr auto
   find(remove_const<CharT> character, idx from_position = 0u) const
      -> maybe<idx> {
      if constexpr (sizeof(remove_const<CharT>) != 1) {
         return this->find_small(character, from_position);
      } else {
         idx const lanes = char1x16::size();
         idx const size = this->m_size;

         // TODO: Tile this loop four or eight times.
         idx i;
         for (i = from_position; i < size && i + lanes < size; i += lanes) {
            // TODO: Consider aligning this load?
            char1x16 const storage =
               make_simd_loaded<char1x16>(this->m_p_data + i);
            // TODO: Support a native ABI mask here.
            auto const mask = storage.equal_lanes(character);
            if (mask.any_of()) {
               return i + mask.find_if_true();
            }
         }

         // The last chunk of this string, smaller than `char1x16::lanes`, is
         // stepped through one character at a time.
         return this->find_small(character, i);
      }
   }

 private:
   // `basic_str_span` inherits:
   //
   // char_type* _Nullable m_p_data;
   // idx m_size;
};

template <typename CharT, idx inline_capacity, str_vec_flags flags>
basic_str_span(basic_str_inplace<CharT, inline_capacity, flags>&)
   -> basic_str_span<CharT, flags.str>;

template <typename CharT, idx inline_capacity, str_vec_flags flags>
basic_str_span(basic_str_inplace<CharT, inline_capacity, flags> const&)
   -> basic_str_span<CharT const, flags.str>;

template <typename CharT, str_vec_flags flags>
basic_str_span(basic_str_vec<CharT, flags>&)
   -> basic_str_span<CharT, flags.str>;

template <typename CharT, str_vec_flags flags>
basic_str_span(basic_str_vec<CharT, flags> const&)
   -> basic_str_span<CharT const, flags.str>;

template <typename String>
concept is_safe_string = requires(String const& string) {
                            typename remove_cvref<String>::value_type;
                            string.data();
                            string.size();
                            string.is_null_terminated();
                         };

template <typename String>
concept is_string_utf8_interconvertible =
   is_safe_string<String>
   && is_char_utf8_interconvertible<typename String::value_type>;

template <is_safe_string Left, is_safe_string Right>
   requires(
      !is_same<remove_cvref<Left>, remove_cvref<Right>>
      && is_same<
         remove_const<typename remove_cvref<Left>::value_type>,
         remove_const<typename remove_cvref<Right>::value_type>>
   )
[[nodiscard]]
constexpr auto
operator==(Left const& left, Right const& right) -> bool {
   using char_type = remove_const<typename remove_cvref<Left>::value_type>;
   using view_type = basic_str_span<char_type const>;
   return compare_strings(view_type(left), view_type(right));
}

template <typename String, typename CharT, idx extent>
   requires(
      is_safe_string<String>
      && is_same<
         remove_const<typename remove_cvref<String>::value_type>,
         remove_const<CharT>>
   )
[[nodiscard]]
constexpr auto
operator==(String const& left, CharT const (&right)[extent]) -> bool {
   using char_type = remove_const<CharT>;
   using view_type = basic_str_span<char_type const>;
   return compare_strings(view_type(left), view_type(right, extent - 1u));
}

}  // namespace cat
