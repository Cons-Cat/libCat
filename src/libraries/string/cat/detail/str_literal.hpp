// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/array>
#include <cat/container>
#include <cat/debug>
#include <cat/meta>
#include <cat/utility>

#include "str_span.hpp"

namespace cat {

namespace detail {
template <auto constant_value>
struct str_literal_constant {
   static constexpr auto value = constant_value;

   constexpr
   operator decltype(constant_value)() const {
      return value;
   }

   [[nodiscard]]
   constexpr auto
   operator()() const -> decltype(constant_value) {
      return value;
   }
};
}  // namespace detail

template <typename CharT, idx fixed_size>
class basic_str_literal;

template <idx size>
using str_literal = basic_str_literal<char, size>;

template <idx size>
using u8str_literal = basic_str_literal<char8_t, size>;

template <idx size>
using u16str_literal = basic_str_literal<char16_t, size>;

template <idx size>
using u32str_literal = basic_str_literal<char32_t, size>;

template <idx size>
using wstr_literal = basic_str_literal<wchar_t, size>;

template <typename CharT, idx fixed_size>
class
   [[clang::preferred_name(str_literal<fixed_size>),
     clang::preferred_name(u8str_literal<fixed_size>),
     clang::preferred_name(u16str_literal<fixed_size>),
     clang::preferred_name(u32str_literal<fixed_size>),
     clang::preferred_name(wstr_literal<fixed_size>)]]
   basic_str_literal
    : public container_interface<
         basic_str_literal<CharT, fixed_size>, CharT const>,
      public random_access_stepanov_iterable_interface<CharT const> {
 public:
   using value_type = CharT;
   using pointer = CharT const*;
   using const_pointer = CharT const*;
   using reference = CharT const&;
   using const_reference = CharT const&;
   using iterator = CharT const*;
   using const_iterator = CharT const*;
   using size_type = idx;
   using difference_type = iword;
   using view_type = basic_str_span<CharT const, false>;

   CharT data_[fixed_size + 1u]{};

   template <typename... Chars>
      requires(
         fixed_size > 0u && sizeof...(Chars) == fixed_size
         && (is_same<remove_cvref<Chars>, CharT> && ...)
      )
   constexpr explicit basic_str_literal(Chars... chars)
       : data_{chars..., CharT{}} {
   }

   template <idx extent>
      requires(extent == fixed_size + 1u)
   consteval basic_str_literal(CharT const (&text)[extent]) {
      [[assume(text[fixed_size.raw] == CharT{})]];
      for (idx index; index <= fixed_size; ++index) {
         data_[index] = text[index];
      }
   }

   template <typename Iterator, typename Sentinel>
      requires(
         requires(Iterator iterator, Sentinel sentinel) {
            *iterator;
            ++iterator;
            iterator != sentinel;
         } && is_same<remove_cvref<decltype(* declval<Iterator&>())>, CharT>
      )
   constexpr basic_str_literal(Iterator begin, Sentinel end) {
      for (idx index; index < fixed_size; ++index) {
         if (begin == end) {
            if consteval {
               __builtin_unreachable();
            } else {
               cat::assert(false);
            }
         }
         data_[index] = *begin;
         ++begin;
      }
      if (begin != end) {
         if consteval {
            __builtin_unreachable();
         } else {
            cat::assert(false);
         }
      }
      data_[fixed_size] = CharT{};
   }

   template <typename Range>
      requires requires(Range const& range) {
                  range.begin();
                  range.end();
                  range.size();
               }
   constexpr explicit basic_str_literal(Range const& range)
       : basic_str_literal(range.begin(), range.end()) {
   }

   constexpr basic_str_literal(basic_str_literal const&) = default;
   constexpr auto
   operator=(basic_str_literal const&) -> basic_str_literal& = default;

   static constexpr detail::str_literal_constant<fixed_size> size{};
   static constexpr detail::str_literal_constant<fixed_size> length{};
   static constexpr detail::str_literal_constant<fixed_size> max_size{};
   static constexpr detail::str_literal_constant<fixed_size == 0u> empty{};

   [[nodiscard]]
   constexpr auto
   begin() const [[clang::lifetimebound]] -> const_iterator {
      return data_;
   }

   [[nodiscard]]
   constexpr auto
   end() const [[clang::lifetimebound]] -> const_iterator {
      return data_ + fixed_size;
   }

   [[nodiscard]]
   constexpr auto
   cbegin() const [[clang::lifetimebound]] -> const_iterator {
      return begin();
   }

   [[nodiscard]]
   constexpr auto
   cend() const [[clang::lifetimebound]] -> const_iterator {
      return end();
   }

   [[nodiscard]]
   constexpr auto
   operator[](idx position) const [[clang::lifetimebound]] -> const_reference {
      return data_[position];
   }

   [[nodiscard]]
   constexpr auto
   at(idx position) const [[clang::lifetimebound]] -> const_reference {
      if !consteval {
         cat::assert(position < fixed_size);
      }
      return data_[position];
   }

   [[nodiscard]]
   constexpr auto
   front() const [[clang::lifetimebound]] -> const_reference
      requires(fixed_size > 0u)
   {
      return data_[0];
   }

   [[nodiscard]]
   constexpr auto
   back() const [[clang::lifetimebound]] -> const_reference
      requires(fixed_size > 0u)
   {
      return data_[fixed_size.raw - 1u];
   }

   constexpr void
   swap(basic_str_literal& other) {
      for (idx index; index <= fixed_size; ++index) {
         cat::swap(data_[index], other.data_[index]);
      }
   }

   [[nodiscard, gnu::returns_nonnull]]
   constexpr auto
   c_str() const [[clang::lifetimebound]] -> const_pointer {
      return data_;
   }

   [[nodiscard, gnu::returns_nonnull]]
   constexpr auto
   data() const [[clang::lifetimebound]] -> const_pointer {
      return data_;
   }

   [[nodiscard]]
   constexpr auto
   view() const [[clang::lifetimebound]] -> view_type {
      return {data_, fixed_size};
   }

   [[nodiscard]]
   constexpr
   operator view_type() const [[clang::lifetimebound]] {
      return view();
   }

   [[nodiscard]]
   static constexpr auto
   is_null_terminated() -> bool {
      return true;
   }

   template <idx other_size>
   [[nodiscard]]
   friend constexpr auto
   operator+(
      basic_str_literal const& left,
      basic_str_literal<CharT, other_size> const& right
   ) -> basic_str_literal<CharT, fixed_size + other_size> {
      CharT joined[fixed_size + other_size]{};
      for (idx index; index < fixed_size; ++index) {
         joined[index] = left[index];
      }
      for (idx index; index < other_size; ++index) {
         joined[fixed_size + index] = right[index];
      }
      return {joined, joined + fixed_size + other_size};
   }

   [[nodiscard]]
   friend constexpr auto
   operator+(basic_str_literal const& left, CharT right)
      -> basic_str_literal<CharT, fixed_size + 1u> {
      CharT joined[fixed_size + 1u]{};
      for (idx index; index < fixed_size; ++index) {
         joined[index] = left[index];
      }
      joined[fixed_size] = right;
      return {joined, joined + fixed_size + 1u};
   }

   [[nodiscard]]
   friend constexpr auto
   operator+(CharT left, basic_str_literal const& right)
      -> basic_str_literal<CharT, fixed_size + 1u> {
      CharT joined[fixed_size + 1u]{left};
      for (idx index; index < fixed_size; ++index) {
         joined[index + 1u] = right[index];
      }
      return {joined, joined + fixed_size + 1u};
   }

   template <idx extent>
   [[nodiscard]]
   consteval friend auto
   operator+(basic_str_literal const& left, CharT const (&right)[extent])
      -> basic_str_literal<CharT, idx(fixed_size + extent - 1u)> {
      [[assume(right[extent.raw - 1u] == CharT{})]];
      CharT joined[fixed_size.raw + extent.raw - 1u]{};
      for (idx index; index < fixed_size; ++index) {
         joined[index] = left[index];
      }
      for (idx index; index < extent - 1u; ++index) {
         joined[fixed_size + index] = right[index];
      }
      return {joined, joined + (fixed_size.raw + extent.raw - 1u)};
   }

   template <idx extent>
   [[nodiscard]]
   consteval friend auto
   operator+(CharT const (&left)[extent], basic_str_literal const& right)
      -> basic_str_literal<CharT, idx(extent - 1u + fixed_size)> {
      [[assume(left[extent.raw - 1u] == CharT{})]];
      CharT joined[extent.raw - 1u + fixed_size.raw]{};
      for (idx index; index < extent - 1u; ++index) {
         joined[index] = left[index];
      }
      for (idx index; index < fixed_size; ++index) {
         joined[extent - 1u + index] = right[index];
      }
      return {joined, joined + (extent.raw - 1u + fixed_size.raw)};
   }

   template <idx other_size>
   [[nodiscard]]
   friend constexpr auto
   operator==(
      basic_str_literal const& left,
      basic_str_literal<CharT, other_size> const& right
   ) -> bool {
      if constexpr (fixed_size != other_size) {
         return false;
      } else {
         for (idx index; index < fixed_size; ++index) {
            if (left[index] != right[index]) {
               return false;
            }
         }
         return true;
      }
   }

   template <idx extent>
   [[nodiscard]]
   consteval friend auto
   operator==(basic_str_literal const& left, CharT const (&right)[extent])
      -> bool {
      [[assume(right[extent.raw - 1u] == CharT{})]];
      if constexpr (fixed_size != extent - 1u) {
         return false;
      } else {
         for (idx index; index < fixed_size; ++index) {
            if (left[index] != right[index]) {
               return false;
            }
         }
         return true;
      }
   }

   template <idx other_size>
   [[nodiscard]]
   friend constexpr auto
   operator<=>(
      basic_str_literal const& left,
      basic_str_literal<CharT, other_size> const& right
   ) -> strong_ordering {
      idx const common_size = fixed_size < other_size ? fixed_size : other_size;
      for (idx index; index < common_size; ++index) {
         if (left[index] < right[index]) {
            return strong_ordering::less;
         }
         if (left[index] > right[index]) {
            return strong_ordering::greater;
         }
      }
      if constexpr (fixed_size < other_size) {
         return strong_ordering::less;
      } else if constexpr (fixed_size > other_size) {
         return strong_ordering::greater;
      } else {
         return strong_ordering::equal;
      }
   }

   template <idx extent>
   [[nodiscard]]
   consteval friend auto
   operator<=>(basic_str_literal const& left, CharT const (&right)[extent])
      -> strong_ordering {
      [[assume(right[extent.raw - 1u] == CharT{})]];
      constexpr idx right_size = idx(extent - 1u);
      idx const common_size = fixed_size < right_size ? fixed_size : right_size;
      for (idx index; index < common_size; ++index) {
         if (left[index] < right[index]) {
            return strong_ordering::less;
         }
         if (left[index] > right[index]) {
            return strong_ordering::greater;
         }
      }
      if constexpr (fixed_size < right_size) {
         return strong_ordering::less;
      } else if constexpr (fixed_size > right_size) {
         return strong_ordering::greater;
      } else {
         return strong_ordering::equal;
      }
   }
};

template <typename CharT, idx size>
constexpr void
swap(
   basic_str_literal<CharT, size>& left, basic_str_literal<CharT, size>& right
) {
   left.swap(right);
}

template <typename CharT, typename... Rest>
   requires((is_same<remove_cvref<Rest>, remove_cvref<CharT>> && ...))
basic_str_literal(CharT, Rest...)
   -> basic_str_literal<remove_cvref<CharT>, sizeof...(Rest) + 1u>;

template <typename CharT, idx extent>
basic_str_literal(CharT const (&)[extent])
   -> basic_str_literal<CharT, idx(extent - 1u)>;

template <typename CharT, idx size>
basic_str_literal(array<CharT, size> const&) -> basic_str_literal<CharT, size>;

}  // namespace cat
