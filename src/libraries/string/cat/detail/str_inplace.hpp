// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// `cat::basic_str_inplace` is a fixed-capacity owning string container that
// stores its characters inline. It is a string analogue to `cat::vec_inplace`
// and closely mirrors the API of both `cat::vec_inplace` and `cat::strvec`.
// Upon construction, the `str_inplace` is empty.
//
// It is parameterized by a character type, its inline capacity, whether the
// string is null-terminated, and `cat::vec_flags`. The `fixed_size` flag makes
// the string a fixed size upon construction with no separate capacity.
//
// Convenience type aliases are provided for these configurations:
//
//  `cat::str_inplace<n>`, a `basic_str_inplace` of up to `n` `char`s.
//
//  `cat::zstr_inplace<n>`, a null-terminated `basic_str_inplace` of up to `n`
//  `char`s.
//
//  `cat::wstr_inplace<n>`, a `basic_str_inplace` of up to `n` `wchar_t`s.
//
//  `cat::wzstr_inplace<n>`, a null-terminated `basic_str_inplace` of up to `n`
//  `wchar_t`s .
//
//  `cat::str_inplace_fixed<n>`, a `str_inplace` of exactly `n` `char`s.
//
//  `cat::zstr_inplace_fixed<n>`, a `zstr_inplace` of exactly `n` `char`s.
//
//  `cat::wstr_inplace_fixed<n>`, a `wstr_inplace` of exactly `n` `wchar_t`s.
//
//  `cat::wzstr_inplace_fixed<n>`, a `wzstr_inplace` of exactly `n` `wchar_t`s.
//
// This class is non-structural, so it cannot be used as a non-type template
// parameter unlike `cat::str_literal`.

#include <cat/math>
#include <cat/maybe>
#include <cat/memory>
#include <cat/span>
#include <cat/utility>

namespace cat {

namespace detail {
template <bool tracks_size>
struct str_inplace_size;

template <>
struct str_inplace_size<true> {
   idx m_size = 0u;
};

template <>
struct str_inplace_size<false> {};
}  // namespace detail

template <
   typename CharT, idx inline_capacity, bool null_terminated,
   vec_flags flags = {}>
class basic_str_inplace;

template <idx inline_capacity, vec_flags flags = {}>
using str_inplace = basic_str_inplace<char, inline_capacity, false, flags>;

template <idx inline_capacity, vec_flags flags = {}>
using zstr_inplace = basic_str_inplace<char, inline_capacity, true, flags>;

template <idx inline_capacity, vec_flags flags = {}>
using wstr_inplace = basic_str_inplace<wchar_t, inline_capacity, false, flags>;

template <idx inline_capacity, vec_flags flags = {}>
using wzstr_inplace = basic_str_inplace<wchar_t, inline_capacity, true, flags>;

template <idx inline_capacity, vec_flags flags = {}>
using str_inplace_fixed = basic_str_inplace<
   char, inline_capacity, false, flags | vec_flags::fixed_size>;

template <idx inline_capacity, vec_flags flags = {}>
using zstr_inplace_fixed = basic_str_inplace<
   char, inline_capacity, true, flags | vec_flags::fixed_size>;

template <idx inline_capacity, vec_flags flags = {}>
using wstr_inplace_fixed = basic_str_inplace<
   wchar_t, inline_capacity, false, flags | vec_flags::fixed_size>;

template <idx inline_capacity, vec_flags flags = {}>
using wzstr_inplace_fixed = basic_str_inplace<
   wchar_t, inline_capacity, true, flags | vec_flags::fixed_size>;

template <
   typename CharT, idx inline_capacity, bool null_terminated, vec_flags flags>
class
   [[clang::trivial_abi,
     clang::preferred_name(str_inplace<inline_capacity, flags>),
     clang::preferred_name(zstr_inplace<inline_capacity, flags>),
     clang::preferred_name(wstr_inplace<inline_capacity, flags>),
     clang::preferred_name(wzstr_inplace<inline_capacity, flags>), gsl::Owner]]
   basic_str_inplace
    : public container_interface<
         basic_str_inplace<CharT, inline_capacity, null_terminated, flags>,
         CharT>,
      public random_access_stepanov_iterable_interface<CharT>,
      private detail::str_inplace_size<!flags.is_fixed_size> {
   static_assert(
      is_same<remove_cvref<CharT>, CharT>,
      "`CharT` must not be cvref-qualified!"
   );

 public:
   constexpr basic_str_inplace() {
      this->write_terminator();
   }

   constexpr basic_str_inplace(basic_str_inplace const& string) = default;

   template <
      idx other_capacity, bool other_null_terminated, vec_flags other_flags>
      requires(!flags.is_fixed_size && other_capacity <= inline_capacity)
   constexpr auto
   operator=(
      basic_str_inplace<
         CharT, other_capacity, other_null_terminated, other_flags> const& other
   ) -> basic_str_inplace& {
      this->m_size = other.size();
      for (idx index; index < other.size(); ++index) {
         m_data[index] = other[index];
      }
      this->write_terminator();
      return *this;
   }

   // Construct from a string literal.
   template <idx extent>
      requires(
         flags.is_fixed_size ? extent == inline_capacity + 1u
                             : extent <= inline_capacity + 1u
      )
   consteval basic_str_inplace(CharT const (&string)[extent]) {
      [[assume(string[extent.raw - 1u] == CharT{'\0'})]];
      idx const content_size = idx(extent - 1u);
      if constexpr (!flags.is_fixed_size) {
         this->m_size = content_size;
      }
      for (idx index; index < content_size; ++index) {
         m_data[index] = string[index];
      }
      this->write_terminator();
   }

   template <
      idx other_capacity, bool other_null_terminated, vec_flags other_flags>
   [[nodiscard]]
   constexpr auto
   operator==(
      basic_str_inplace<
         CharT, other_capacity, other_null_terminated, other_flags> const& other
   ) const -> bool {
      if (size() != other.size()) {
         return false;
      }
      for (idx index; index < size(); ++index) {
         if (m_data[index] != other[index]) {
            return false;
         }
      }
      return true;
   }

   template <
      idx other_capacity, bool other_null_terminated, vec_flags other_flags>
   [[nodiscard]]
   constexpr auto
   operator<=>(
      basic_str_inplace<
         CharT, other_capacity, other_null_terminated, other_flags> const& other
   ) const {
      return span<CharT const>(data(), size())
             <=> span<CharT const>(other.data(), other.size());
   }

   template <idx extent>
   [[nodiscard]]
   constexpr auto
   operator==(CharT const (&other)[extent]) const -> bool {
      idx const other_size = idx(extent - 1u);
      if (size() != other_size) {
         return false;
      }
      for (idx index; index < size(); ++index) {
         if (m_data[index] != other[index]) {
            return false;
         }
      }
      return true;
   }

   template <idx extent>
   [[nodiscard]]
   friend constexpr auto
   operator==(CharT const (&left)[extent], basic_str_inplace const& right)
      -> bool {
      return right == left;
   }

   constexpr auto
   operator==(CharT const other) const -> bool
      requires(inline_capacity == 1u)
   {
      return size() == 1u && m_data[0] == other;
   }

   constexpr void
   swap(basic_str_inplace& other) {
      for (idx index; index < inline_capacity + null_terminated; ++index) {
         cat::swap(m_data[index], other.m_data[index]);
      }
      if constexpr (!flags.is_fixed_size) {
         cat::swap(this->m_size, other.m_size);
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
      if constexpr (flags.is_fixed_size) {
         return inline_capacity;
      } else {
         return this->m_size;
      }
   }

   [[nodiscard]]
   static constexpr auto
   capacity() -> idx {
      return inline_capacity;
   }

   [[nodiscard]]
   static constexpr auto
   max_size() -> idx {
      return inline_capacity;
   }

   [[nodiscard]]
   constexpr auto
   is_null_terminated() const -> bool {
      if constexpr (null_terminated) {
         return true;
      } else if (size() == 0u) {
         return false;
      } else {
         return m_data[size() - 1u] == CharT{'\0'};
      }
   }

   [[nodiscard]]
   constexpr auto
   c_str() const [[clang::lifetimebound]]
   -> CharT const* _Nonnull requires(null_terminated) { return m_data; }

   [[nodiscard]] constexpr auto reserve(idx minimum_capacity) const
      -> maybe<void> {
      if (minimum_capacity > inline_capacity) {
         return nullopt;
      }
      return monostate;
   }

   [[nodiscard]]
   constexpr auto
   resize(idx new_size, CharT value = CharT{'\0'}) -> maybe<void>
      requires(!flags.is_fixed_size)
   {
      if (new_size > inline_capacity) {
         return nullopt;
      }
      for (idx index = size(); index < new_size; ++index) {
         m_data[index] = value;
      }
      this->m_size = new_size;
      this->write_terminator();
      return monostate;
   }

   [[nodiscard]]
   constexpr auto
   push_back(CharT value) -> maybe<CharT&>
      requires(!flags.is_fixed_size)
   {
      if (size() == inline_capacity) {
         return nullopt;
      }
      return unchecked_push_back(value);
   }

   constexpr auto
   unchecked_push_back(CharT value) -> CharT&
      requires(!flags.is_fixed_size)
   {
      cat::assert(size() < inline_capacity);
      CharT& result = m_data[this->m_size];
      result = value;
      ++this->m_size;
      this->write_terminator();
      return result;
   }

   template <idx extent>
   [[nodiscard]]
   constexpr auto
   append(CharT const (&string)[extent]) -> maybe<void>
      requires(!flags.is_fixed_size)
   {
      idx const count = idx(extent - 1u);
      if (count > inline_capacity - size()) {
         return nullopt;
      }
      for (idx index; index < count; ++index) {
         m_data[this->m_size + index] = string[index];
      }
      this->m_size += count;
      this->write_terminator();
      return monostate;
   }

   template <typename Range>
      requires(has_size<Range> && !flags.is_fixed_size)
   [[nodiscard]]
   constexpr auto
   append_range(Range&& range) -> maybe<void> {
      idx const count = range.size();
      if (count > inline_capacity - size()) {
         return nullopt;
      }
      for (auto&& value : range) {
         unchecked_push_back($fwd(value));
      }
      return monostate;
   }

   constexpr auto
   fill(CharT value) -> basic_str_inplace& {
      for (idx index; index < size(); ++index) {
         m_data[index] = value;
      }
      return *this;
   }

   [[nodiscard]]
   constexpr auto
   pop_back() -> maybe<CharT>
      requires(!flags.is_fixed_size)
   {
      if (this->is_empty()) {
         return nullopt;
      }
      this->m_size = idx(this->m_size - 1u);
      CharT const result = m_data[this->m_size];
      this->write_terminator();
      return result;
   }

   constexpr void
   clear()
      requires(!flags.is_fixed_size)
   {
      this->m_size = 0u;
      this->write_terminator();
   }

   constexpr void
   erase(idx index)
      requires(!flags.is_fixed_size)
   {
      cat::assert(index < size());
      for (idx source = index + 1u; source < size(); ++source) {
         m_data[idx(source - 1u)] = m_data[source];
      }
      this->m_size = idx(this->m_size - 1u);
      this->write_terminator();
   }

   constexpr void
   erase(idx first, idx last)
      requires(!flags.is_fixed_size)
   {
      cat::assert(first <= last);
      cat::assert(last <= size());
      idx const count = idx(last - first);
      for (idx source = last; source < size(); ++source) {
         m_data[idx(source - count)] = m_data[source];
      }
      this->m_size = idx(this->m_size - count);
      this->write_terminator();
   }

   template <idx other_capacity, vec_flags other_flags>
   friend constexpr auto
   operator+(
      basic_str_inplace const& self,
      basic_str_inplace<
         CharT, other_capacity, null_terminated, other_flags> const&
         other_string
   )
      -> basic_str_inplace<
         CharT, inline_capacity + other_capacity, null_terminated> {
      basic_str_inplace<
         CharT, inline_capacity + other_capacity, null_terminated>
         new_string;
      for (idx i; i < self.size(); ++i) {
         new_string.unchecked_push_back(self.m_data[i]);
      }
      for (idx i; i < other_string.size(); ++i) {
         new_string.unchecked_push_back(other_string[i]);
      }
      return new_string;
   }

   template <idx other_length>
   friend constexpr auto
   operator+(
      basic_str_inplace const& self, CharT const (&other_string)[other_length]
   ) {
      [[assume(other_string[other_length.raw - 1u] == '\0')]];
      return self
             + basic_str_inplace<
                CharT, idx(other_length - 1u), null_terminated>{other_string};
   }

   template <idx other_length>
   friend constexpr auto
   operator+(
      CharT const (&other_string)[other_length], basic_str_inplace const& self
   ) {
      [[assume(other_string[other_length.raw - 1u] == '\0')]];
      return basic_str_inplace<CharT, idx(other_length - 1u), null_terminated>{
                other_string
             }
             + self;
   }

   friend constexpr auto
   operator+(basic_str_inplace const& self, CharT value)
      -> basic_str_inplace<CharT, inline_capacity + 1u, null_terminated> {
      basic_str_inplace<CharT, inline_capacity + 1u, null_terminated> result;
      for (idx index; index < self.size(); ++index) {
         result.unchecked_push_back(self[index]);
      }
      result.unchecked_push_back(value);
      return result;
   }

   friend constexpr auto
   operator+(CharT value, basic_str_inplace const& self)
      -> basic_str_inplace<CharT, inline_capacity + 1u, null_terminated> {
      basic_str_inplace<CharT, inline_capacity + 1u, null_terminated> result;
      result.unchecked_push_back(value);
      for (idx index; index < self.size(); ++index) {
         result.unchecked_push_back(self[index]);
      }
      return result;
   }

 private:
   constexpr void
   write_terminator() {
      if constexpr (null_terminated) {
         m_data[size()] = CharT{'\0'};
      }
   }

   static constexpr __SIZE_TYPE__ storage_size =
      inline_capacity + null_terminated == 0u
         ? 1u
         : inline_capacity + null_terminated;

   CharT m_data[storage_size]{};
};

template <typename T, typename CharT>
struct formatter;

// Implementing this here is a circular dependency. The implementation can be
// found in <cat/string/implementations/format_str_inplace.tpp>.
template <
   idx inline_capacity, bool null_terminated, vec_flags flags, typename CharT>
   requires(is_same<CharT, char>)
struct formatter<
   basic_str_inplace<char, inline_capacity, null_terminated, flags>, CharT>;

// Deduce the length of string literals without a null-terminator.
template <idx len>
basic_str_inplace(char const (&str)[len])
   -> basic_str_inplace<char, idx(len - 1u), false, vec_flags{}>;

template <idx len>
basic_str_inplace(wchar_t const (&str)[len])
   -> basic_str_inplace<wchar_t, idx(len - 1u), false, vec_flags{}>;

template <idx padded_length, idx deduced_length>
   requires((deduced_length - 1u) <= padded_length)
consteval auto
make_str_inplace(char const (&string)[deduced_length])
   -> str_inplace<padded_length> {
   [[assume(string[deduced_length.raw - 1u] == '\0')]];
   str_inplace<padded_length> new_string;
   auto _ = new_string.append(string);
   return new_string;
}

template <idx padded_length, idx deduced_length>
   requires((deduced_length - 1u) <= padded_length)
consteval auto
make_wstr_inplace(wchar_t const (&string)[deduced_length])
   -> wstr_inplace<padded_length> {
   [[assume(string[deduced_length.raw - 1u] == L'\0')]];
   wstr_inplace<padded_length> new_string;
   auto _ = new_string.append(string);
   return new_string;
}

template <idx padded_length, idx deduced_length>
   requires((deduced_length - 1u) <= padded_length)
consteval auto
make_zstr_inplace(char const (&string)[deduced_length])
   -> zstr_inplace<padded_length> {
   [[assume(string[deduced_length.raw - 1u] == '\0')]];
   zstr_inplace<padded_length> new_string;
   auto _ = new_string.append(string);
   return new_string;
}

template <idx padded_length, idx deduced_length>
   requires((deduced_length - 1u) <= padded_length)
consteval auto
make_wzstr_inplace(wchar_t const (&string)[deduced_length])
   -> wzstr_inplace<padded_length> {
   [[assume(string[deduced_length.raw - 1u] == L'\0')]];
   wzstr_inplace<padded_length> new_string;
   auto _ = new_string.append(string);
   return new_string;
}

template <idx inline_capacity>
[[nodiscard]]
constexpr auto
make_str_inplace_filled(idx count, char value)
   -> maybe<str_inplace<inline_capacity>> {
   str_inplace<inline_capacity> result;
   $prop(result.resize(count, value));
   return result;
}

template <idx inline_capacity>
[[nodiscard]]
constexpr auto
make_zstr_inplace_filled(idx count, char value)
   -> maybe<zstr_inplace<inline_capacity>> {
   zstr_inplace<inline_capacity> result;
   $prop(result.resize(count, value));
   return result;
}

template <idx inline_capacity>
[[nodiscard]]
constexpr auto
make_wstr_inplace_filled(idx count, wchar_t value)
   -> maybe<wstr_inplace<inline_capacity>> {
   wstr_inplace<inline_capacity> result;
   $prop(result.resize(count, value));
   return result;
}

template <idx inline_capacity>
[[nodiscard]]
constexpr auto
make_wzstr_inplace_filled(idx count, wchar_t value)
   -> maybe<wzstr_inplace<inline_capacity>> {
   wzstr_inplace<inline_capacity> result;
   $prop(result.resize(count, value));
   return result;
}

template <idx fixed_size>
[[nodiscard]]
constexpr auto
make_str_inplace_fixed_filled(char value) -> str_inplace_fixed<fixed_size> {
   str_inplace_fixed<fixed_size> result;
   result.fill(value);
   return result;
}

template <idx fixed_size>
[[nodiscard]]
constexpr auto
make_zstr_inplace_fixed_filled(char value) -> zstr_inplace_fixed<fixed_size> {
   zstr_inplace_fixed<fixed_size> result;
   result.fill(value);
   return result;
}

template <idx fixed_size>
[[nodiscard]]
constexpr auto
make_wstr_inplace_fixed_filled(wchar_t value)
   -> wstr_inplace_fixed<fixed_size> {
   wstr_inplace_fixed<fixed_size> result;
   result.fill(value);
   return result;
}

template <idx fixed_size>
[[nodiscard]]
constexpr auto
make_wzstr_inplace_fixed_filled(wchar_t value)
   -> wzstr_inplace_fixed<fixed_size> {
   wzstr_inplace_fixed<fixed_size> result;
   result.fill(value);
   return result;
}

}  // namespace cat
