// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/math>
#include <cat/memory>
#include <cat/span>
#include <cat/utility>

namespace cat {

template <typename CharT, idx length, bool null_terminated>
class basic_str_inplace;

template <idx deduced_length>
using str_inplace = basic_str_inplace<char, deduced_length, false>;

template <idx deduced_length>
using zstr_inplace = basic_str_inplace<char, deduced_length, true>;

template <idx deduced_length>
using wstr_inplace = basic_str_inplace<wchar_t, deduced_length, false>;

template <idx deduced_length>
using wzstr_inplace = basic_str_inplace<wchar_t, deduced_length, true>;

template <typename CharT, idx length, bool null_terminated>
class
   [[clang::preferred_name(str_inplace<length>),
     clang::preferred_name(zstr_inplace<length>),
     clang::preferred_name(wstr_inplace<length>),
     clang::preferred_name(wzstr_inplace<length>), gsl::Owner]]
   basic_str_inplace
    : public container_interface<
         basic_str_inplace<CharT, length, null_terminated>, CharT>,
      public random_access_stepanov_iterable_interface<CharT> {
 public:
   constexpr basic_str_inplace() = default;

   constexpr basic_str_inplace(basic_str_inplace const& string) = default;

   // Construct and deduce length from a string literal. Add 1 to length to
   // ignore a null terminator.
   consteval basic_str_inplace(
      CharT const (
         &string
      )[idx(length.raw + static_cast<__SIZE_TYPE__>(!null_terminated))]
   ) {
      [[assume(
         string[length.raw - static_cast<__SIZE_TYPE__>(null_terminated)]
         == CharT{'\0'}
      )]];
      // This must be copied instead of initialized in-place to guarantee
      // `const`-correctness.
      this->copy_string_data(string);
   }

   // Assign a `basic_str_inplace` of lesser or equal length.
   constexpr auto
   // NOLINTNEXTLINE This does handle self-assignment.
   operator=(basic_str_inplace<CharT, length, null_terminated> const& string)
      -> basic_str_inplace<CharT, length, null_terminated>& {
      this->copy_string_data(string.data());
      return *this;
   }

   // TODO: Make this `consteval`.
   // Assign a string literal of lesser or equal length.
   consteval auto
   operator=(CharT const (
      &string
   )[idx(length.raw - static_cast<__SIZE_TYPE__>(null_terminated))])
      -> basic_str_inplace& {
      [[assume(
         string[length.raw - 1uz + static_cast<__SIZE_TYPE__>(!null_terminated)]
         == CharT{'\0'}
      )]];
      this->copy_string_data(string);
      return *this;
   }

   // Compare two strings of equal length.
   constexpr auto
   operator==(basic_str_inplace const& other) const -> bool {
      for (idx i = 0; i < length; ++i) {
         if (m_data[i] != other[i]) {
            return false;
         }
      }
      return true;
   };

   constexpr auto
   operator==(CharT const other) const -> bool
      requires(length == 1u + static_cast<unsigned>(null_terminated))
   {
      return m_data[0] == other;
   }

   constexpr void
   swap(basic_str_inplace& other) {
      for (idx index = 0u; index < length; ++index) {
         cat::swap(m_data[index], other.m_data[index]);
      }
   }

   [[gnu::returns_nonnull]]
   constexpr auto
   data() [[clang::lifetimebound]] -> CharT* _Nonnull {
      return m_data;
   }

   [[nodiscard, gnu::returns_nonnull]]
   constexpr auto
   data() const [[clang::lifetimebound]] -> CharT const* _Nonnull {
      return m_data;
   }

   [[nodiscard]]
   constexpr auto
   size() const -> idx {
      return length;
   }

   [[nodiscard]]
   constexpr auto
   is_null_terminated() const -> bool {
      if constexpr (null_terminated) {
         return true;
      } else if constexpr (length == 0u) {
         return false;
      } else {
         return m_data[length - 1u] == CharT{'\0'};
      }
   }

   // Concatenate constant-evaluated strings.
   template <idx other_length>
   friend constexpr auto
   operator+(
      basic_str_inplace<CharT, length, null_terminated> const& self,
      basic_str_inplace<CharT, other_length, null_terminated> const&
         other_string
   )
      -> basic_str_inplace<
         CharT,
         narrow_cast<idx>(
            uword{length} + uword{other_length} - (null_terminated ? 1u : 0u)
         )
            .value(),
         null_terminated> {
      // `zstr_inplace` stores a trailing null in `length`, so `length` is at
      // least one when `is_null_terminated` subtracts one. Otherwise the
      // subtrahend is zero. The concatenated size drops at most one duplicate
      // null, never more than `length + other_length`.
      constexpr idx const self_useful =
         narrow_cast<idx>(uword{length} - (null_terminated ? 1u : 0u)).value();
      constexpr idx const new_length =
         narrow_cast<idx>(
            uword{length} + uword{other_length} - (null_terminated ? 1u : 0u)
         )
            .value();

      basic_str_inplace<CharT, new_length, null_terminated> new_string;

      for (idx i = 0u; i < self_useful; ++i) {
         new_string[i] = self.m_data[i];
      }

      for (idx i = self_useful; i < new_length; ++i) {
         new_string[i] = other_string.data()[i.raw - self_useful.raw];
      }

      return new_string;
   }

   // Concatenate a string literal to a `basic_str_inplace`.
   template <idx other_length>
   friend constexpr auto
   operator+(
      basic_str_inplace<CharT, length, null_terminated> const& self,
      CharT const (&other_string)[other_length]
   ) {
      [[assume(other_string[other_length.raw - 1] == '\0')]];
      // The assume implies `other_length` is at least one when the literal is
      // treated as null-terminated (`!is_null_terminated` is false). Otherwise
      // the subtrahend is zero.
      return self
             + basic_str_inplace<
                CharT,
                narrow_cast<idx>(other_length - (null_terminated ? 0u : 1u))
                   .value(),
                null_terminated>{other_string};
   }

   // Concatenate a `basic_str_inplace` to a string literal.
   template <idx other_length>
   friend constexpr auto
   operator+(
      CharT const (&other_string)[other_length],
      basic_str_inplace<CharT, length, null_terminated> const& self
   ) {
      [[assume(other_string[other_length.raw - 1] == '\0')]];
      // Same reasoning as `operator+` from `basic_str_inplace` to a literal.
      return basic_str_inplace<
                CharT,
                narrow_cast<idx>(other_length - (null_terminated ? 0u : 1u))
                   .value(),
                null_terminated>{other_string}
             + self;
   }

 private:
   constexpr void
   copy_string_data(CharT const* _Nonnull p_source) {
      if consteval {
         for (idx i = 0u; i < length; ++i) {
            this->m_data[i] = p_source[i];
         }
      } else {
         copy_memory(p_source, this->data(), length * sizeof(CharT));
      }
   }

   CharT m_data[length];
};

// Deduce the length of string literals without a null-terminator.
template <idx len>
basic_str_inplace(char const (&str)[len])
   // A string literal includes a trailing null, so `len` is at least one.
   ->basic_str_inplace<char, narrow_cast<idx>(len - 1u).value(), false>;

template <idx len>
basic_str_inplace(wchar_t const (&str)[len])
   // A string literal includes a trailing null, so `len` is at least one.
   ->basic_str_inplace<wchar_t, narrow_cast<idx>(len - 1u).value(), false>;

// Create a `str_inplace` from a smaller string, and null out the unfilled
// bytes.
template <idx padded_length, idx deduced_length>
   requires((deduced_length - 1u) <= padded_length)
consteval auto
make_str_inplace(char const (&string)[deduced_length])
   -> str_inplace<padded_length> {
   [[assume(string[deduced_length.raw - 1uz] == '\0')]];
   str_inplace<padded_length> new_string;
   // The assume matches a normal string literal, so `deduced_length` is at
   // least one and `deduced_length - 1u` is the last character index, not an
   // underflowed length.
   for (idx i; i < narrow_cast<idx>(deduced_length - 1u).value(); ++i) {
      new_string[i] = string[i.raw];
   }
   // Pad the string out with null bytes. Padding starts at the same index as
   // the literal's trailing null slot, so the starting index matches the copy
   // loop bound above.
   for (idx i = narrow_cast<idx>(deduced_length - 1u).value();
        i < padded_length; ++i) {
      new_string[i] = '\0';
   }
   return new_string;
}

template <idx padded_length, idx deduced_length>
   requires((deduced_length - 1u) <= padded_length)
consteval auto
make_wstr_inplace(wchar_t const (&string)[deduced_length])
   -> wstr_inplace<padded_length> {
   [[assume(string[deduced_length.raw - 1uz] == L'\0')]];
   wstr_inplace<padded_length> new_string;
   for (idx i; i < narrow_cast<idx>(deduced_length - 1u).value(); ++i) {
      new_string[i] = string[i.raw];
   }
   for (idx i = narrow_cast<idx>(deduced_length - 1u).value();
        i < padded_length; ++i) {
      new_string[i] = L'\0';
   }
   return new_string;
}

// TODO: Add a variant that doesn't require `padded_length`.
// Create a `str_inplace` from a smaller string, and null out the unfilled
// bytes.
template <idx padded_length, idx deduced_length>
   requires((deduced_length - 1u) <= padded_length)
consteval auto
make_zstr_inplace(char const (&string)[deduced_length])
   -> zstr_inplace<padded_length> {
   [[assume(string[deduced_length.raw - 1uz] == '\0')]];
   zstr_inplace<padded_length> new_string;
   for (idx i; i < deduced_length; ++i) {
      new_string[i] = string[i];
   }
   // Pad the string out with null bytes.
   for (idx i = deduced_length; i < padded_length; ++i) {
      new_string[i] = '\0';
   }
   return new_string;
}

template <idx padded_length, idx deduced_length>
   requires((deduced_length - 1u) <= padded_length)
consteval auto
make_wzstr_inplace(wchar_t const (&string)[deduced_length])
   -> wzstr_inplace<padded_length> {
   [[assume(string[deduced_length.raw - 1uz] == L'\0')]];
   wzstr_inplace<padded_length> new_string;
   for (idx i; i < deduced_length; ++i) {
      new_string[i] = string[i];
   }
   for (idx i = deduced_length; i < padded_length; ++i) {
      new_string[i] = L'\0';
   }
   return new_string;
}

}  // namespace cat
