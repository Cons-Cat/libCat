// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/simd_impl.hpp>

#include <cat/maybe>
#include <cat/simd>
#include <cat/span>
#include <cat/utility>

#include "str_inplace.hpp"

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

template <typename CharT, bool is_null_terminated>
class basic_str_span;

inline namespace manual {
template <typename CharT, bool is_null_terminated>
class basic_str_vec;
}  // namespace manual

using str_view = basic_str_span<char const, false>;
using str_span = basic_str_span<char, false>;
using zstr_view = basic_str_span<char const, true>;
using zstr_span = basic_str_span<char, true>;

using wstr_view = basic_str_span<wchar_t const, false>;
using wstr_span = basic_str_span<wchar_t, false>;
using wzstr_view = basic_str_span<wchar_t const, true>;
using wzstr_span = basic_str_span<wchar_t, true>;

template <typename CharT>
[[nodiscard]]
constexpr auto
compare_strings_scalar(
   basic_str_span<CharT const, false> string_1,
   basic_str_span<CharT const, false> string_2
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
   basic_str_span<CharT const, false> string_1,
   basic_str_span<CharT const, false> string_2
) -> bool {
   return compare_strings_scalar(string_1, string_2);
}

[[nodiscard]]
constexpr auto
compare_strings(str_view string_1, str_view string_2) -> bool;

template <typename CharT, bool null_terminated>
class
   [[clang::preferred_name(str_view), clang::preferred_name(str_span),
     clang::preferred_name(zstr_view), clang::preferred_name(zstr_span),
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

   // This weird template deduces lower than the string literal constructor.
   template <is_pointer T>
   constexpr basic_str_span(T _Nonnull p_string [[clang::lifetimebound]])
       : span<CharT>(p_string, string_length(p_string) + null_terminated) {
   }

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
      this->m_size =
         narrow_cast<idx>(other_length - (null_terminated ? 0u : 1u)).value();
   }

   template <idx other_length>
      requires(!is_const<CharT>)
   consteval basic_str_span(CharT const (&string)[other_length]) =
      delete ("Cannot construct a mutable `str_span` over a `char const*` "
              "literal! Consider a `str_view` instead.");

   // Make a `str_span` over a `str_inplace`.
   template <idx other_length>
   constexpr basic_str_span(
      basic_str_inplace<CharT, other_length, null_terminated>& other_string
      [[clang::lifetimebound]]
   ) {
      this->m_p_data = other_string.data();
      this->m_size = other_length;
   }

   // Make a `str_view` over a `str_inplace`.
   template <idx other_length>
      requires(is_const<CharT>)
   constexpr basic_str_span(
      basic_str_inplace<
         remove_const<CharT>, other_length, null_terminated> const& other_string
      [[clang::lifetimebound]]
   ) {
      this->m_p_data = other_string.data();
      this->m_size = other_length;
   }

   // Make a `str_span` over a `zstr_inplace`.
   template <typename T, idx other_length>
      requires(!null_terminated)
   constexpr basic_str_span(
      basic_str_inplace<T, other_length, true>& other_string
      [[clang::lifetimebound]]
   ) {
      this->m_p_data = other_string.data();
      // `zstr_inplace` storage length counts the trailing null, so it is never
      // zero and `other_length - 1u` cannot wrap for a non-empty string.
      this->m_size = narrow_cast<idx>(other_length - 1u).value();
   }

   // Make a `str_view` over a `zstr_inplace`.
   template <typename T, idx other_length>
      requires(!null_terminated)
   constexpr basic_str_span(
      basic_str_inplace<T, other_length, true> const& other_string
      [[clang::lifetimebound]]
   ) {
      this->m_p_data = other_string.data();
      // Same bound as the mutable overload above.
      this->m_size = narrow_cast<idx>(other_length - 1u).value();
   }

   // Make a `str_span` over a `zstr_inplace`.
   template <idx other_length>
      requires(null_terminated && !is_const<CharT>)
   constexpr basic_str_span(
      basic_str_inplace<CharT, other_length, false> const&
   ) = delete ("Cannot bind a null-terminated `zstr_span` over a "
               "non null-terminated `str_inplace`!");

   template <bool other_is_null_terminated>
      requires(!is_const<CharT> && null_terminated == other_is_null_terminated)
   constexpr basic_str_span(
      basic_str_vec<CharT, other_is_null_terminated>& other_string
      [[clang::lifetimebound]]
   ) {
      this->m_p_data = other_string.data();
      this->m_size = other_string.size();
   }

   template <bool other_is_null_terminated>
      requires(is_const<CharT> && null_terminated == other_is_null_terminated)
   constexpr basic_str_span(
      basic_str_vec<remove_const<CharT>, other_is_null_terminated> const&
         other_string [[clang::lifetimebound]]
   ) {
      this->m_p_data = other_string.data();
      this->m_size = other_string.size();
   }

   template <bool other_is_null_terminated>
      requires(!is_const<CharT> && !null_terminated && other_is_null_terminated)
   constexpr basic_str_span(
      basic_str_vec<CharT, other_is_null_terminated>& other_string
      [[clang::lifetimebound]]
   ) {
      this->m_p_data = other_string.data();
      this->m_size =
         other_string.size() == 0u ? 0u : idx(other_string.size() - 1u);
   }

   template <bool other_is_null_terminated>
      requires(is_const<CharT> && !null_terminated && other_is_null_terminated)
   constexpr basic_str_span(
      basic_str_vec<remove_const<CharT>, other_is_null_terminated> const&
         other_string [[clang::lifetimebound]]
   ) {
      this->m_p_data = other_string.data();
      this->m_size =
         other_string.size() == 0u ? 0u : idx(other_string.size() - 1u);
   }

   // Promote `basic_str_span<char>` to `basic_str_span<char const>`.
   template <typename T>
      requires(is_same<CharT, add_const<T>>)
   constexpr basic_str_span(
      basic_str_span<T, null_terminated> in_span [[clang::lifetimebound]]
   )
       : span<CharT>(in_span) {
   }

   // Promote `zstr_span` to `str_span`.
   template <typename T>
      requires(!is_same<CharT, add_const<T>> && !null_terminated)
   constexpr basic_str_span(
      basic_str_span<T, true> in_span [[clang::lifetimebound]]
   ) {
      this->m_p_data = in_span.data();
      // `zstr_span` size counts the trailing null, so it is never zero here.
      this->m_size = narrow_cast<idx>(in_span.size() - 1u).value();
   }

   // Promote `zstr_span<char>` to `str_span<char const>`.
   template <typename T>
      requires(is_same<CharT, add_const<T>> && !null_terminated)
   constexpr basic_str_span(
      basic_str_span<T, true> in_span [[clang::lifetimebound]]
   ) {
      this->m_p_data = in_span.data();
      // Same as the non-`const` promotion above.
      this->m_size = narrow_cast<idx>(in_span.size() - 1u).value();
   }

   // Prevent binding a `zstr_span` over a `str_span`.
   constexpr basic_str_span(basic_str_span<CharT, false> const&)
      requires(null_terminated)
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
      )
   constexpr basic_str_span(
      String& string [[clang::lifetimebound]]
   ) {
      this->m_p_data = string.data();
      this->m_size = string.size();
      bool const source_is_null_terminated = string.is_null_terminated();
      if constexpr (null_terminated) {
         if !consteval {
            assert(source_is_null_terminated);
         }
      } else if (source_is_null_terminated && this->m_size != 0u) {
         this->m_size = idx(this->m_size - 1u);
      }
   }

   template <typename String>
      requires(
         is_const<CharT>
         && is_same<
            remove_const<CharT>, typename remove_cvref<String>::value_type>
      )
   constexpr basic_str_span(
      String const& string [[clang::lifetimebound]]
   ) {
      this->m_p_data = string.data();
      this->m_size = string.size();
      bool const source_is_null_terminated = string.is_null_terminated();
      if constexpr (null_terminated) {
         if !consteval {
            assert(source_is_null_terminated);
         }
      } else if (source_is_null_terminated && this->m_size != 0u) {
         this->m_size = idx(this->m_size - 1u);
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
      if constexpr (null_terminated) {
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
      return compare_strings(this_string, other_string);
   }

   // TODO: Make these member functions `const`.

   [[nodiscard]]
   constexpr auto
   substring(idx position, idx count) -> basic_str_span {
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
      return this->substring(offset, idx(this->m_size - offset));
   }

   [[nodiscard]]
   constexpr auto
   remove_suffix(idx offset) -> basic_str_span {
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

template <typename CharT, idx length, bool null_terminated>
basic_str_span(basic_str_inplace<CharT, length, null_terminated>&)
   -> basic_str_span<CharT, null_terminated>;

template <typename CharT, idx length, bool null_terminated>
basic_str_span(basic_str_inplace<CharT, length, null_terminated> const&)
   -> basic_str_span<CharT const, null_terminated>;

template <typename CharT, bool null_terminated>
basic_str_span(basic_str_vec<CharT, null_terminated>&)
   -> basic_str_span<CharT, null_terminated>;

template <typename CharT, bool null_terminated>
basic_str_span(basic_str_vec<CharT, null_terminated> const&)
   -> basic_str_span<CharT const, null_terminated>;

}  // namespace cat
