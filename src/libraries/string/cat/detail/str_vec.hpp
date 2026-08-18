// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// `cat::basic_str_vec` is a dynamically-sized owning string container backed by
// a `cat::basic_vec` of characters. It is inspired by `std::basic_string`. Upon
// construction, the `str_vec` is empty. Its allocation and growth behavior is
// configured by `cat::str_vec_flags`.
//
// As this is a `manual` namespace container, its destructor does not deallocate
// storage. To deallocate storage, call `.free(allocator)` or
// `.cfree(allocator)`.
//
// The `cat::raii::basic_str_vec` deallocates its own memory automatically.
//
// It is parameterized by a character type and `cat::str_vec_flags`.
//
// Convenience type aliases are provided:
//
//  `cat::str_vec`, a `basic_str_vec` of `char`.
//  `cat::zstr_vec`, a null-terminated `basic_str_vec` of `char`.
//  `cat::u8str_vec` / `zu8str_vec`, likewise for `char8_t`.
//  `cat::u16str_vec` / `zu16str_vec`, likewise for `char16_t`.
//  `cat::u32str_vec` / `zu32str_vec`, likewise for `char32_t`.
//  `cat::wstr_vec`, a `basic_str_vec` of `wchar_t`.
//  `cat::wzstr_vec`, a null-terminated `basic_str_vec` of `wchar_t`.

#include <cat/iterable>
#include <cat/vec>

#include "./str_vec_flags.hpp"
#include "./str_span.hpp"

namespace cat {

namespace raii {
template <typename CharT, str_vec_flags flags, is_allocator Allocator>
class basic_str_vec;
}  // namespace raii

inline namespace manual {

template <typename CharT, str_vec_flags flags = vec_flags::pointer_size_layout>
class basic_str_vec;

using str_vec = basic_str_vec<char>;
using zstr_vec = basic_str_vec<
   char, str_flags::null_terminated | vec_flags::pointer_size_layout>;
using u8str_vec = basic_str_vec<char8_t>;
using zu8str_vec = basic_str_vec<
   char8_t, str_flags::null_terminated | vec_flags::pointer_size_layout>;
using u16str_vec = basic_str_vec<char16_t>;
using zu16str_vec = basic_str_vec<
   char16_t, str_flags::null_terminated | vec_flags::pointer_size_layout>;
using u32str_vec = basic_str_vec<char32_t>;
using zu32str_vec = basic_str_vec<
   char32_t, str_flags::null_terminated | vec_flags::pointer_size_layout>;
using wstr_vec = basic_str_vec<wchar_t>;
using wzstr_vec = basic_str_vec<
   wchar_t, str_flags::null_terminated | vec_flags::pointer_size_layout>;

template <str_vec_flags flags = vec_flags::pointer_size_layout>
using str_vec_fixed = basic_str_vec<char, vec_flags::fixed_size | flags>;

template <str_vec_flags flags = vec_flags::pointer_size_layout>
using zstr_vec_fixed = basic_str_vec<
   char, str_flags::null_terminated | vec_flags::fixed_size | flags>;

template <str_vec_flags flags = vec_flags::pointer_size_layout>
using u8str_vec_fixed = basic_str_vec<char8_t, vec_flags::fixed_size | flags>;

template <str_vec_flags flags = vec_flags::pointer_size_layout>
using zu8str_vec_fixed = basic_str_vec<
   char8_t, str_flags::null_terminated | vec_flags::fixed_size | flags>;

template <str_vec_flags flags = vec_flags::pointer_size_layout>
using u16str_vec_fixed = basic_str_vec<char16_t, vec_flags::fixed_size | flags>;

template <str_vec_flags flags = vec_flags::pointer_size_layout>
using zu16str_vec_fixed = basic_str_vec<
   char16_t, str_flags::null_terminated | vec_flags::fixed_size | flags>;

template <str_vec_flags flags = vec_flags::pointer_size_layout>
using u32str_vec_fixed = basic_str_vec<char32_t, vec_flags::fixed_size | flags>;

template <str_vec_flags flags = vec_flags::pointer_size_layout>
using zu32str_vec_fixed = basic_str_vec<
   char32_t, str_flags::null_terminated | vec_flags::fixed_size | flags>;

template <str_vec_flags flags = vec_flags::pointer_size_layout>
using wstr_vec_fixed = basic_str_vec<wchar_t, vec_flags::fixed_size | flags>;

template <str_vec_flags flags = vec_flags::pointer_size_layout>
using wzstr_vec_fixed = basic_str_vec<
   wchar_t, str_flags::null_terminated | vec_flags::fixed_size | flags>;

template <
   idx inline_count = 16u, str_vec_flags flags = vec_flags::pointer_size_layout>
using small_str_vec =
   basic_str_vec<char, vec_flags::inline_storage(inline_count) | flags>;

template <
   idx inline_count = 16u, str_vec_flags flags = vec_flags::pointer_size_layout>
using small_zstr_vec = basic_str_vec<
   char, str_flags::null_terminated
            | vec_flags::inline_storage(inline_count)
            | flags>;

template <
   idx inline_count = 16u, str_vec_flags flags = vec_flags::pointer_size_layout>
using small_u8str_vec =
   basic_str_vec<char8_t, vec_flags::inline_storage(inline_count) | flags>;

template <
   idx inline_count = 16u, str_vec_flags flags = vec_flags::pointer_size_layout>
using small_zu8str_vec = basic_str_vec<
   char8_t, str_flags::null_terminated
               | vec_flags::inline_storage(inline_count)
               | flags>;

template <
   idx inline_count = 16u, str_vec_flags flags = vec_flags::pointer_size_layout>
using small_u16str_vec =
   basic_str_vec<char16_t, vec_flags::inline_storage(inline_count) | flags>;

template <
   idx inline_count = 16u, str_vec_flags flags = vec_flags::pointer_size_layout>
using small_zu16str_vec = basic_str_vec<
   char16_t, str_flags::null_terminated
                | vec_flags::inline_storage(inline_count)
                | flags>;

template <
   idx inline_count = 16u, str_vec_flags flags = vec_flags::pointer_size_layout>
using small_u32str_vec =
   basic_str_vec<char32_t, vec_flags::inline_storage(inline_count) | flags>;

template <
   idx inline_count = 16u, str_vec_flags flags = vec_flags::pointer_size_layout>
using small_zu32str_vec = basic_str_vec<
   char32_t, str_flags::null_terminated
                | vec_flags::inline_storage(inline_count)
                | flags>;

template <
   idx inline_count = 16u, str_vec_flags flags = vec_flags::pointer_size_layout>
using small_wstr_vec =
   basic_str_vec<wchar_t, vec_flags::inline_storage(inline_count) | flags>;

template <
   idx inline_count = 16u, str_vec_flags flags = vec_flags::pointer_size_layout>
using small_wzstr_vec = basic_str_vec<
   wchar_t, str_flags::null_terminated
               | vec_flags::inline_storage(inline_count)
               | flags>;

template <
   idx inline_count = 16u, str_vec_flags flags = vec_flags::pointer_size_layout>
using small_str_vec_fixed = basic_str_vec<
   char,
   vec_flags::inline_storage(inline_count) | vec_flags::fixed_size | flags>;

template <
   idx inline_count = 16u, str_vec_flags flags = vec_flags::pointer_size_layout>
using small_zstr_vec_fixed = basic_str_vec<
   char, str_flags::null_terminated
            | vec_flags::inline_storage(inline_count)
            | vec_flags::fixed_size
            | flags>;

template <
   idx inline_count = 16u, str_vec_flags flags = vec_flags::pointer_size_layout>
using small_u8str_vec_fixed = basic_str_vec<
   char8_t,
   vec_flags::inline_storage(inline_count) | vec_flags::fixed_size | flags>;

template <
   idx inline_count = 16u, str_vec_flags flags = vec_flags::pointer_size_layout>
using small_zu8str_vec_fixed = basic_str_vec<
   char8_t, str_flags::null_terminated
               | vec_flags::inline_storage(inline_count)
               | vec_flags::fixed_size
               | flags>;

template <
   idx inline_count = 16u, str_vec_flags flags = vec_flags::pointer_size_layout>
using small_u16str_vec_fixed = basic_str_vec<
   char16_t,
   vec_flags::inline_storage(inline_count) | vec_flags::fixed_size | flags>;

template <
   idx inline_count = 16u, str_vec_flags flags = vec_flags::pointer_size_layout>
using small_zu16str_vec_fixed = basic_str_vec<
   char16_t, str_flags::null_terminated
                | vec_flags::inline_storage(inline_count)
                | vec_flags::fixed_size
                | flags>;

template <
   idx inline_count = 16u, str_vec_flags flags = vec_flags::pointer_size_layout>
using small_u32str_vec_fixed = basic_str_vec<
   char32_t,
   vec_flags::inline_storage(inline_count) | vec_flags::fixed_size | flags>;

template <
   idx inline_count = 16u, str_vec_flags flags = vec_flags::pointer_size_layout>
using small_zu32str_vec_fixed = basic_str_vec<
   char32_t, str_flags::null_terminated
                | vec_flags::inline_storage(inline_count)
                | vec_flags::fixed_size
                | flags>;

template <
   idx inline_count = 16u, str_vec_flags flags = vec_flags::pointer_size_layout>
using small_wstr_vec_fixed = basic_str_vec<
   wchar_t,
   vec_flags::inline_storage(inline_count) | vec_flags::fixed_size | flags>;

template <
   idx inline_count = 16u, str_vec_flags flags = vec_flags::pointer_size_layout>
using small_wzstr_vec_fixed = basic_str_vec<
   wchar_t, str_flags::null_terminated
               | vec_flags::inline_storage(inline_count)
               | vec_flags::fixed_size
               | flags>;

}  // namespace manual

namespace detail {
template <typename CharT, str_vec_flags flags>
constexpr auto
maybe_str_vec_has_value(basic_str_vec<CharT, flags> const& value) -> bool;

template <typename CharT, str_vec_flags flags>
constexpr auto
maybe_str_vec_nullopt() -> basic_str_vec<CharT, flags>;
}  // namespace detail

inline namespace manual {

template <typename CharT, str_vec_flags configuration>
class
   [[clang::preferred_name(str_vec), clang::preferred_name(zstr_vec),
     clang::preferred_name(u8str_vec), clang::preferred_name(zu8str_vec),
     clang::preferred_name(u16str_vec), clang::preferred_name(zu16str_vec),
     clang::preferred_name(u32str_vec), clang::preferred_name(zu32str_vec),
     clang::preferred_name(wstr_vec), clang::preferred_name(wzstr_vec),
     gsl::Owner]]
   basic_str_vec
    : public container_interface<basic_str_vec<CharT, configuration>, CharT>,
      public random_access_stepanov_iterable_interface<CharT> {
 public:
   static constexpr str_vec_flags flags = configuration;

 private:
   static_assert(
      is_same<remove_cvref<CharT>, CharT>,
      "`CharT` must not be cvref-qualified!"
   );

   template <
      typename OtherChar, str_vec_flags other_flags, is_allocator Allocator>
   friend class raii::basic_str_vec;

 public:
   constexpr basic_str_vec() = default;
   constexpr basic_str_vec(basic_str_vec&&) = default;

   constexpr basic_str_vec(basic_str_vec const&) =
      delete ("Implicit copying of `cat::basic_str_vec` is forbidden. Call "
              "`.clone()` or move instead!");

   auto
   operator=(basic_str_vec const&) -> basic_str_vec& =
      delete ("Implicit copying of `cat::basic_str_vec` is forbidden. Call "
              "`.clone()` or move instead!");

   constexpr auto
   operator=(basic_str_vec&&) -> basic_str_vec& = default;

   [[nodiscard]]
   constexpr auto
   operator==(basic_str_vec const& rhs) const -> bool {
      return compare_strings(this->view(), rhs.view());
   }

   template <str_flags other_flags>
   [[nodiscard]]
   constexpr auto
   operator==(basic_str_span<CharT const, other_flags> rhs) const -> bool {
      return compare_strings(this->view(), basic_str_span<CharT const>(rhs));
   }

   constexpr auto
   fill(CharT value) -> basic_str_vec& {
      for (idx index; index < size(); ++index) {
         data()[index] = value;
      }
      return *this;
   }

   constexpr void
   swap(basic_str_vec& other) {
      m_core.swap(other.m_core);
   }

   [[nodiscard]]
   constexpr auto
   data() [[clang::lifetimebound]] -> CharT* _Nullable {
      return m_core.data();
   }

   [[nodiscard]]
   constexpr auto
   data() const [[clang::lifetimebound]] -> CharT const* _Nullable {
      return m_core.data();
   }

   [[nodiscard]]
   constexpr auto
   size() const -> idx {
      if constexpr (!flags.str.is_null_terminated) {
         return m_core.size();
      } else {
         return m_core.size() == 0u ? 0u : idx(m_core.size() - 1u);
      }
   }

   [[nodiscard]]
   constexpr auto
   is_null_terminated() const -> bool {
      if constexpr (flags.str.is_null_terminated) {
         return true;
      } else {
         return size() != 0u && m_core[idx(size() - 1u)] == CharT{'\0'};
      }
   }

   [[nodiscard]]
   constexpr auto
   capacity() const -> idx {
      if constexpr (!flags.str.is_null_terminated) {
         return m_core.capacity();
      } else {
         return m_core.capacity() == 0u ? 0u : idx(m_core.capacity() - 1u);
      }
   }

   [[nodiscard]]
   constexpr auto
   view() const [[clang::lifetimebound]] -> basic_str_span<CharT const> {
      if (m_core.data() == nullptr || size() == 0u) {
         return {};
      }
      return {m_core.data(), size()};
   }

   [[nodiscard]]
   constexpr auto
   span() [[clang::lifetimebound]] -> basic_str_span<CharT, flags.str> {
      if (m_core.data() == nullptr) {
         return {};
      }
      return {
         m_core.data(),
         size() + static_cast<unsigned>(flags.str.is_null_terminated),
      };
   }

   [[nodiscard]]
   constexpr auto
   span() const [[clang::lifetimebound]]
   -> basic_str_span<CharT const, flags.str> {
      if (m_core.data() == nullptr) {
         return {};
      }
      return {
         m_core.data(),
         size() + static_cast<unsigned>(flags.str.is_null_terminated),
      };
   }

   [[nodiscard]]
   constexpr
   operator basic_str_span<CharT, flags.str>() [[clang::lifetimebound]] {
      return this->span();
   }

   [[nodiscard]]
   constexpr
   operator basic_str_span<CharT const, flags.str>() const
      [[clang::lifetimebound]] {
      return this->span();
   }

   template <is_allocator Allocator>
   [[clang::reinitializes]]
   constexpr void
   free(allocator_ref<Allocator> allocator) {
      m_core.free(allocator);
   }

   [[clang::reinitializes, gnu::always_inline, gnu::nodebug]]
   constexpr void
   free(dyn_allocator allocator) {
      return free<dyn_allocator>(allocator);
   }

   template <is_allocator Allocator>
   [[clang::reinitializes]]
   constexpr void
   cfree(allocator_ref<Allocator> allocator) {
      m_core.cfree(allocator);
   }

   [[clang::reinitializes, gnu::always_inline, gnu::nodebug]]
   constexpr void
   cfree(dyn_allocator allocator) {
      return cfree<dyn_allocator>(allocator);
   }

   [[clang::reinitializes]]
   constexpr void
   reset() {
      this->clear();
   }

   template <is_allocator Allocator>
   [[nodiscard]]
   constexpr auto
   reserve(allocator_ref<Allocator> allocator, idx minimum_capacity)
      -> maybe<void>
      requires(!flags.vec.is_fixed_size)
   {
      return m_core.reserve(
         allocator,
         minimum_capacity + static_cast<unsigned>(flags.str.is_null_terminated)
      );
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   reserve(dyn_allocator allocator, idx minimum_capacity) -> maybe<void>
      requires(!flags.vec.is_fixed_size)
   {
      return reserve<dyn_allocator>(allocator, minimum_capacity);
   }

   template <is_allocator Allocator>
   [[nodiscard]]
   constexpr auto
   resize(allocator_ref<Allocator> allocator, idx new_size) -> maybe<void> {
      return resize(allocator, new_size, CharT{'\0'});
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   resize(dyn_allocator allocator, idx new_size) -> maybe<void> {
      return resize<dyn_allocator>(allocator, new_size);
   }

   template <is_allocator Allocator>
   [[nodiscard]]
   constexpr auto
   resize(allocator_ref<Allocator> allocator, idx new_size, CharT value)
      -> maybe<void> {
      if constexpr (!flags.str.is_null_terminated) {
         return m_core.resize(allocator, new_size, value);
      } else {
         idx const old_content_size = content_size();
         $prop(m_core.resize(allocator, new_size + 1u, value));
         for (idx i = old_content_size; i < new_size; ++i) {
            m_core[i] = value;
         }
         m_core[new_size] = CharT{'\0'};
         return monostate;
      }
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   resize(dyn_allocator allocator, idx new_size, CharT value) -> maybe<void> {
      return resize<dyn_allocator>(allocator, new_size, value);
   }

   template <is_allocator Allocator>
   [[nodiscard]]
   constexpr auto
   shrink_to_fit(allocator_ref<Allocator> allocator) -> maybe<void> {
      return m_core.shrink_to_fit(allocator);
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shrink_to_fit(dyn_allocator allocator) -> maybe<void> {
      return shrink_to_fit<dyn_allocator>(allocator);
   }

   template <is_allocator Allocator>
   [[nodiscard]]
   constexpr auto
   push_back(allocator_ref<Allocator> allocator, CharT value) -> maybe<void> {
      if constexpr (!flags.str.is_null_terminated) {
         return m_core.push_back(allocator, value);
      } else {
         if (m_core.size() == 0u) {
            $prop(m_core.reserve(allocator, 2u));
            $prop(m_core.push_back(allocator, value));
            $prop(m_core.push_back(allocator, CharT{'\0'}));
         } else {
            $prop(m_core.reserve(allocator, m_core.size() + 1u));
            m_core[idx(m_core.size() - 1u)] = value;
            $prop(m_core.push_back(allocator, CharT{'\0'}));
         }
         return monostate;
      }
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   push_back(dyn_allocator allocator, CharT value) -> maybe<void> {
      return push_back<dyn_allocator>(allocator, value);
   }

   template <
      is_allocator Allocator, typename First, typename Second,
      typename... Remaining>
   constexpr auto
   push_back(allocator_ref<Allocator>, First&&, Second&&, Remaining&&...)
      -> maybe<void> = delete (
         "`push_back()` is not variadic! Consider either `emplace_back()` "
         "to construct an element or `append_range()` to "
         "append a range."
      );

   template <typename First, typename Second, typename... Remaining>
   constexpr auto
   push_back(dyn_allocator, First&&, Second&&, Remaining&&...) -> maybe<void> =
      delete ("`push_back()` is not variadic! Consider either `emplace_back()` "
              "to construct an element or `append_range()` to "
              "append a range.");

   [[nodiscard]]
   constexpr auto
   pop_back() -> maybe<CharT> {
      if constexpr (!flags.str.is_null_terminated) {
         return m_core.pop_back();
      } else {
         idx const current_content_size = content_size();
         if (current_content_size == 0u) {
            return nullopt;
         }
         CharT const result = m_core[idx(current_content_size - 1u)];
         this->erase_raw(idx(current_content_size - 1u), current_content_size);
         return result;
      }
   }

   [[clang::reinitializes]]
   constexpr void
   clear() {
      if constexpr (!flags.str.is_null_terminated) {
         m_core.reset();
      } else {
         if (m_core.size() == 0u) {
            return;
         }
         this->erase_raw(0u, content_size());
      }
   }

   constexpr void
   erase(idx index) {
      if constexpr (flags.str.is_null_terminated) {
         cat::assert(index < content_size());
      }
      this->erase_raw(index, index + 1u);
   }

   constexpr void
   erase(idx start, idx end) {
      if constexpr (flags.str.is_null_terminated) {
         cat::assert(end <= content_size());
      }
      this->erase_raw(start, end);
   }

   template <is_allocator Allocator>
   [[nodiscard]]
   constexpr auto
   append(
      allocator_ref<Allocator> allocator, basic_str_span<CharT const> string
   ) -> maybe<void> {
      if constexpr (!flags.str.is_null_terminated) {
         return m_core.append_range(allocator, string);
      } else {
         if (string.size() == 0u && m_core.size() != 0u) {
            return monostate;
         }
         idx const old_content_size = content_size();
         $prop(
            m_core.reserve(allocator, old_content_size + string.size() + 1u)
         );
         if (m_core.size() != 0u) {
            auto _ = m_core.pop_back();
         }
         for (CharT value : string) {
            $prop(m_core.push_back(allocator, value));
         }
         $prop(m_core.push_back(allocator, CharT{'\0'}));
         return monostate;
      }
   }

   template <is_allocator Allocator>
   [[nodiscard]]
   constexpr auto
   append(
      allocator_ref<Allocator> allocator,
      basic_str_span<CharT const, str_flags::null_terminated> string
   ) -> maybe<void> {
      return append(allocator, basic_str_span<CharT const>(string));
   }

   template <is_allocator Allocator, typename OtherChar, idx extent>
      requires(encoding_compatible_char<CharT, OtherChar>)
   [[nodiscard]]
   constexpr auto
   append(allocator_ref<Allocator> allocator, OtherChar const (&string)[extent])
      -> maybe<void> {
      basic_str_inplace<CharT, idx(extent - 1u)> converted;
      $prop(converted.append(string));
      return append(allocator, basic_str_span<CharT const>(converted));
   }

   template <is_allocator Allocator, typename OtherChar, idx extent>
      requires(is_string_char<OtherChar>
               && !encoding_compatible_char<CharT, OtherChar>)
   constexpr auto
   append(allocator_ref<Allocator>, OtherChar const (&)[extent])
      -> maybe<void> = delete ("Cannot copy between different character "
                               "encodings! Transcode the string first.");

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   append(dyn_allocator allocator, basic_str_span<CharT const> string)
      -> maybe<void> {
      return append<dyn_allocator>(allocator, string);
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   append(
      dyn_allocator allocator,
      basic_str_span<CharT const, str_flags::null_terminated> string
   ) -> maybe<void> {
      return append<dyn_allocator>(allocator, string);
   }

   template <typename OtherChar, idx extent>
      requires(encoding_compatible_char<CharT, OtherChar>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   append(dyn_allocator allocator, OtherChar const (&string)[extent])
      -> maybe<void> {
      return append<dyn_allocator>(allocator, string);
   }

   template <typename OtherChar, idx extent>
      requires(is_string_char<OtherChar>
               && !encoding_compatible_char<CharT, OtherChar>)
   constexpr auto
   append(dyn_allocator, OtherChar const (&)[extent])
      -> maybe<void> = delete ("Cannot copy between different character "
                               "encodings! Transcode the string first.");

   // Append every element of `range`.
   template <is_allocator Allocator, is_iterable Iterable>
   [[nodiscard]]
   constexpr auto
   append_range(allocator_ref<Allocator> allocator, Iterable&& range)
      -> maybe<void> {
      if constexpr (!flags.str.is_null_terminated) {
         return m_core.append_range(allocator, $fwd(range));
      } else {
         if (m_core.size() == 0u) {
            $prop(m_core.push_back(allocator, CharT{'\0'}));
         }

         return m_core.insert_range(allocator, content_size(), $fwd(range));
      }
   }

   // Append every element of `range`.
   template <is_iterable Iterable>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   append_range(dyn_allocator allocator, Iterable&& range) -> maybe<void> {
      return append_range<dyn_allocator>(allocator, $fwd(range));
   }

   // Insert every element of `range` before `position`.
   template <is_allocator Allocator, is_iterable Iterable>
   [[nodiscard]]
   constexpr auto
   insert_range(
      allocator_ref<Allocator> allocator, idx position, Iterable&& range
   ) -> maybe<void> {
      if constexpr (flags.str.is_null_terminated) {
         cat::assert(position <= content_size());
      }

      if constexpr (flags.str.is_null_terminated) {
         if (m_core.size() == 0u) {
            $prop(m_core.push_back(allocator, CharT{'\0'}));
         }
      }

      return m_core.insert_range(allocator, position, $fwd(range));
   }

   // Insert every element of `range` before `position`.
   template <is_iterable Iterable>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   insert_range(dyn_allocator allocator, idx position, Iterable&& range)
      -> maybe<void> {
      return insert_range<dyn_allocator>(allocator, position, $fwd(range));
   }

   // Replace `[first, last)` with every element of `range`.
   template <is_allocator Allocator, is_iterable Iterable>
   [[nodiscard]]
   constexpr auto
   replace_with_range(
      allocator_ref<Allocator> allocator, idx first, idx last, Iterable&& range
   ) -> maybe<void> {
      if constexpr (flags.str.is_null_terminated) {
         cat::assert(first <= last);
         cat::assert(last <= content_size());
         if (m_core.size() == 0u) {
            $prop(m_core.push_back(allocator, CharT{'\0'}));
         }
      }

      return m_core.replace_with_range(allocator, first, last, $fwd(range));
   }

   // Replace `[first, last)` with every element of `range`.
   template <is_iterable Iterable>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   replace_with_range(
      dyn_allocator allocator, idx first, idx last, Iterable&& range
   ) -> maybe<void> {
      return replace_with_range<dyn_allocator>(
         allocator, first, last, $fwd(range)
      );
   }

   template <is_allocator NewAllocator>
   [[nodiscard]]
   constexpr auto
   clone(allocator_ref<NewAllocator> allocator) const& -> maybe<basic_str_vec> {
      basic_str_vec new_string;
      new_string.m_core = $prop(m_core.clone(allocator));
      return new_string;
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   clone(dyn_allocator allocator) const& -> maybe<basic_str_vec> {
      return clone<dyn_allocator>(allocator);
   }

   [[nodiscard]]
   static constexpr auto
   max_size() -> idx {
      return cat::basic_vec<CharT, flags.vec>::max_size()
             - static_cast<unsigned>(flags.str.is_null_terminated);
   }

 private:
   [[nodiscard]]
   constexpr auto
   content_size() const -> idx {
      return size();
   }

   constexpr void
   erase_raw(idx start, idx end) {
      cat::assert(start <= end);
      cat::assert(end <= m_core.size());
      auto const count = end - start;
      if (count == 0u) {
         return;
      }

      for (idx i = end; i < m_core.size(); ++i) {
         m_core[idx(i - count)] = m_core[i];
      }
      for (idx i = 0u; i < count; ++i) {
         auto _ = m_core.pop_back();
      }
   }

   template <typename OtherCharT, str_vec_flags other_flags>
   friend constexpr auto
   detail::maybe_str_vec_has_value(
      basic_str_vec<OtherCharT, other_flags> const&
   ) -> bool;

   template <typename OtherCharT, str_vec_flags other_flags>
   friend constexpr auto
   detail::maybe_str_vec_nullopt() -> basic_str_vec<OtherCharT, other_flags>;

   cat::basic_vec<CharT, flags.vec> m_core;
};

}  // namespace manual

template <typename T, typename CharT>
struct formatter;

// Implementing this here is a circular dependency. The implementation can be
// found in <cat/string/implementations/format_str_vec.tpp>.
template <typename CharT, str_vec_flags flags>
   requires(is_char_utf8_interconvertible<CharT>)
struct formatter<basic_str_vec<CharT, flags>, char>;

namespace detail {
template <typename CharT, str_vec_flags flags>
constexpr auto
maybe_str_vec_has_value(basic_str_vec<CharT, flags> const& value) -> bool {
   return maybe_vec_has_value(value.m_core);
}

template <typename CharT, str_vec_flags flags>
constexpr auto
maybe_str_vec_nullopt() -> basic_str_vec<CharT, flags> {
   basic_str_vec<CharT, flags> value;
   value.m_core = maybe_vec_nullopt<CharT, flags.vec>();
   return value;
}

template <
   typename CharT, str_vec_flags flags, is_allocator Allocator, typename First,
   typename... Rest>
constexpr auto
append_str_vec_parts(
   basic_str_vec<CharT, flags>& string, allocator_ref<Allocator> allocator,
   First const& first, Rest const&... rest
) -> maybe<void> {
   basic_str_span<CharT const> const view = basic_str_span(first);
   $prop(string.append(allocator, view));
   if constexpr (sizeof...(Rest) == 0u) {
      return monostate;
   } else {
      return append_str_vec_parts(string, allocator, rest...);
   }
}

template <
   typename CharT, str_vec_flags flags, is_allocator Allocator,
   typename... Strings>
[[nodiscard]]
constexpr auto
make_basic_str_vec(
   allocator_ref<Allocator> allocator, Strings const&... strings
) -> maybe<basic_str_vec<CharT, flags>> {
   basic_str_vec<CharT, flags> new_string;
   idx content_size = 0u;
   ((content_size +=
     basic_str_span<CharT const>(basic_str_span(strings)).size()),
    ...);
   $prop(new_string.reserve(
      allocator, max(content_size, new_string.flags.vec.initial_growth_count)
   ));
   $prop(append_str_vec_parts(new_string, allocator, strings...));
   return new_string;
}

}  // namespace detail

template <typename CharT, str_vec_flags flags>
struct default_compact_trait<basic_str_vec<CharT, flags>>
    : identity_trait<compact<
         basic_str_vec<CharT, flags>,
         detail::maybe_str_vec_has_value<CharT, flags>,
         detail::maybe_str_vec_nullopt<CharT, flags>>> {};

inline namespace manual {

template <
   is_allocator Allocator, str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard]]
constexpr auto
make_str_vec(allocator_ref<Allocator> allocator, str_view string)
   -> maybe<basic_str_vec<char, flags>> {
   basic_str_vec<char, flags> new_string;
   $prop(new_string.reserve(
      allocator, max(string.size(), new_string.flags.vec.initial_growth_count)
   ));
   $prop(new_string.append(allocator, string));
   return new_string;
}

template <str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_str_vec(dyn_allocator allocator, str_view string)
   -> maybe<basic_str_vec<char, flags>> {
   return make_str_vec<dyn_allocator, flags>(allocator, string);
}

template <
   is_allocator Allocator, str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard]]
constexpr auto
make_zstr_vec(allocator_ref<Allocator> allocator, str_view string)
   -> maybe<basic_str_vec<char, str_flags::null_terminated | flags>> {
   basic_str_vec<char, str_flags::null_terminated | flags> new_string;
   $prop(new_string.reserve(
      allocator, max(string.size(), new_string.flags.vec.initial_growth_count)
   ));
   $prop(new_string.append(allocator, string));
   return new_string;
}

template <str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_zstr_vec(dyn_allocator allocator, str_view string)
   -> maybe<basic_str_vec<char, str_flags::null_terminated | flags>> {
   return make_zstr_vec<dyn_allocator, flags>(allocator, string);
}

template <
   is_allocator Allocator, str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard]]
constexpr auto
make_wstr_vec(allocator_ref<Allocator> allocator, wstr_view string)
   -> maybe<basic_str_vec<wchar_t, flags>> {
   basic_str_vec<wchar_t, flags> new_string;
   $prop(new_string.reserve(
      allocator, max(string.size(), new_string.flags.vec.initial_growth_count)
   ));
   $prop(new_string.append(allocator, string));
   return new_string;
}

template <str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wstr_vec(dyn_allocator allocator, wstr_view string)
   -> maybe<basic_str_vec<wchar_t, flags>> {
   return make_wstr_vec<dyn_allocator, flags>(allocator, string);
}

template <
   is_allocator Allocator, str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard]]
constexpr auto
make_wzstr_vec(allocator_ref<Allocator> allocator, wstr_view string)
   -> maybe<basic_str_vec<wchar_t, str_flags::null_terminated | flags>> {
   basic_str_vec<wchar_t, str_flags::null_terminated | flags> new_string;
   $prop(new_string.reserve(
      allocator, max(string.size(), new_string.flags.vec.initial_growth_count)
   ));
   $prop(new_string.append(allocator, string));
   return new_string;
}

template <str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wzstr_vec(dyn_allocator allocator, wstr_view string)
   -> maybe<basic_str_vec<wchar_t, str_flags::null_terminated | flags>> {
   return make_wzstr_vec<dyn_allocator, flags>(allocator, string);
}

template <
   is_allocator Allocator, str_vec_flags flags = vec_flags::pointer_size_layout,
   typename First, typename Second, typename... Strings>
[[nodiscard]]
constexpr auto
make_str_vec(
   allocator_ref<Allocator> allocator, First const& first, Second const& second,
   Strings const&... strings
) -> maybe<basic_str_vec<char, flags>> {
   return detail::make_basic_str_vec<char, flags, Allocator>(
      allocator, first, second, strings...
   );
}

template <
   str_vec_flags flags = vec_flags::pointer_size_layout, typename First,
   typename Second, typename... Strings>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_str_vec(
   dyn_allocator allocator, First const& first, Second const& second,
   Strings const&... strings
) -> maybe<basic_str_vec<char, flags>> {
   return detail::make_basic_str_vec<char, flags, dyn_allocator>(
      allocator, first, second, strings...
   );
}

template <
   is_allocator Allocator, str_vec_flags flags = vec_flags::pointer_size_layout,
   typename First, typename Second, typename... Strings>
[[nodiscard]]
constexpr auto
make_zstr_vec(
   allocator_ref<Allocator> allocator, First const& first, Second const& second,
   Strings const&... strings
) -> maybe<basic_str_vec<char, str_flags::null_terminated | flags>> {
   return detail::make_basic_str_vec<
      char, str_flags::null_terminated | flags, Allocator>(
      allocator, first, second, strings...
   );
}

template <
   str_vec_flags flags = vec_flags::pointer_size_layout, typename First,
   typename Second, typename... Strings>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_zstr_vec(
   dyn_allocator allocator, First const& first, Second const& second,
   Strings const&... strings
) -> maybe<basic_str_vec<char, str_flags::null_terminated | flags>> {
   return detail::make_basic_str_vec<
      char, str_flags::null_terminated | flags, dyn_allocator>(
      allocator, first, second, strings...
   );
}

template <
   is_allocator Allocator, str_vec_flags flags = vec_flags::pointer_size_layout,
   typename First, typename Second, typename... Strings>
[[nodiscard]]
constexpr auto
make_wstr_vec(
   allocator_ref<Allocator> allocator, First const& first, Second const& second,
   Strings const&... strings
) -> maybe<basic_str_vec<wchar_t, flags>> {
   return detail::make_basic_str_vec<wchar_t, flags, Allocator>(
      allocator, first, second, strings...
   );
}

template <
   str_vec_flags flags = vec_flags::pointer_size_layout, typename First,
   typename Second, typename... Strings>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wstr_vec(
   dyn_allocator allocator, First const& first, Second const& second,
   Strings const&... strings
) -> maybe<basic_str_vec<wchar_t, flags>> {
   return detail::make_basic_str_vec<wchar_t, flags, dyn_allocator>(
      allocator, first, second, strings...
   );
}

template <
   is_allocator Allocator, str_vec_flags flags = vec_flags::pointer_size_layout,
   typename First, typename Second, typename... Strings>
[[nodiscard]]
constexpr auto
make_wzstr_vec(
   allocator_ref<Allocator> allocator, First const& first, Second const& second,
   Strings const&... strings
) -> maybe<basic_str_vec<wchar_t, str_flags::null_terminated | flags>> {
   return detail::make_basic_str_vec<
      wchar_t, str_flags::null_terminated | flags, Allocator>(
      allocator, first, second, strings...
   );
}

template <
   str_vec_flags flags = vec_flags::pointer_size_layout, typename First,
   typename Second, typename... Strings>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wzstr_vec(
   dyn_allocator allocator, First const& first, Second const& second,
   Strings const&... strings
) -> maybe<basic_str_vec<wchar_t, str_flags::null_terminated | flags>> {
   return detail::make_basic_str_vec<
      wchar_t, str_flags::null_terminated | flags, dyn_allocator>(
      allocator, first, second, strings...
   );
}

template <
   is_allocator Allocator, str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard]]
constexpr auto
make_str_vec(allocator_ref<Allocator> allocator, initializer_list<char> values)
   -> maybe<basic_str_vec<char, flags>> {
   basic_str_vec<char, flags> new_string;
   $prop(new_string.reserve(
      allocator, max(values.size(), new_string.flags.vec.initial_growth_count)
   ));
   $prop(new_string.append_range(allocator, values));
   return new_string;
}

template <str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_str_vec(dyn_allocator allocator, initializer_list<char> values)
   -> maybe<basic_str_vec<char, flags>> {
   return make_str_vec<dyn_allocator, flags>(allocator, values);
}

template <
   is_allocator Allocator, str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard]]
constexpr auto
make_zstr_vec(allocator_ref<Allocator> allocator, initializer_list<char> values)
   -> maybe<basic_str_vec<char, str_flags::null_terminated | flags>> {
   basic_str_vec<char, str_flags::null_terminated | flags> new_string;
   $prop(new_string.reserve(
      allocator, max(values.size(), new_string.flags.vec.initial_growth_count)
   ));
   $prop(new_string.append_range(allocator, values));
   return new_string;
}

template <str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_zstr_vec(dyn_allocator allocator, initializer_list<char> values)
   -> maybe<basic_str_vec<char, str_flags::null_terminated | flags>> {
   return make_zstr_vec<dyn_allocator, flags>(allocator, values);
}

template <
   is_allocator Allocator, str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard]]
constexpr auto
make_wstr_vec(
   allocator_ref<Allocator> allocator, initializer_list<wchar_t> values
) -> maybe<basic_str_vec<wchar_t, flags>> {
   basic_str_vec<wchar_t, flags> new_string;
   $prop(new_string.reserve(
      allocator, max(values.size(), new_string.flags.vec.initial_growth_count)
   ));
   $prop(new_string.append_range(allocator, values));
   return new_string;
}

template <str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wstr_vec(dyn_allocator allocator, initializer_list<wchar_t> values)
   -> maybe<basic_str_vec<wchar_t, flags>> {
   return make_wstr_vec<dyn_allocator, flags>(allocator, values);
}

template <
   is_allocator Allocator, str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard]]
constexpr auto
make_wzstr_vec(
   allocator_ref<Allocator> allocator, initializer_list<wchar_t> values
) -> maybe<basic_str_vec<wchar_t, str_flags::null_terminated | flags>> {
   basic_str_vec<wchar_t, str_flags::null_terminated | flags> new_string;
   $prop(new_string.reserve(
      allocator, max(values.size(), new_string.flags.vec.initial_growth_count)
   ));
   $prop(new_string.append_range(allocator, values));
   return new_string;
}

template <str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wzstr_vec(dyn_allocator allocator, initializer_list<wchar_t> values)
   -> maybe<basic_str_vec<wchar_t, str_flags::null_terminated | flags>> {
   return make_wzstr_vec<dyn_allocator, flags>(allocator, values);
}

template <
   is_allocator Allocator, str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard]]
constexpr auto
make_str_vec_reserved(allocator_ref<Allocator> allocator, idx capacity)
   -> maybe<basic_str_vec<char, flags>> {
   basic_str_vec<char, flags> new_string;
   $prop(new_string.reserve(
      allocator, max(capacity, new_string.flags.vec.initial_growth_count)
   ));
   return new_string;
}

template <str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_str_vec_reserved(dyn_allocator allocator, idx capacity)
   -> maybe<basic_str_vec<char, flags>> {
   return make_str_vec_reserved<dyn_allocator, flags>(allocator, capacity);
}

template <
   is_allocator Allocator, str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard]]
constexpr auto
make_zstr_vec_reserved(allocator_ref<Allocator> allocator, idx capacity)
   -> maybe<basic_str_vec<char, str_flags::null_terminated | flags>> {
   basic_str_vec<char, str_flags::null_terminated | flags> new_string;
   $prop(new_string.reserve(
      allocator, max(capacity, new_string.flags.vec.initial_growth_count)
   ));
   $prop(new_string.resize(allocator, 0u));
   return new_string;
}

template <str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_zstr_vec_reserved(dyn_allocator allocator, idx capacity)
   -> maybe<basic_str_vec<char, str_flags::null_terminated | flags>> {
   return make_zstr_vec_reserved<dyn_allocator, flags>(allocator, capacity);
}

template <
   is_allocator Allocator, str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard]]
constexpr auto
make_wstr_vec_reserved(allocator_ref<Allocator> allocator, idx capacity)
   -> maybe<basic_str_vec<wchar_t, flags>> {
   basic_str_vec<wchar_t, flags> new_string;
   $prop(new_string.reserve(
      allocator, max(capacity, new_string.flags.vec.initial_growth_count)
   ));
   return new_string;
}

template <str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wstr_vec_reserved(dyn_allocator allocator, idx capacity)
   -> maybe<basic_str_vec<wchar_t, flags>> {
   return make_wstr_vec_reserved<dyn_allocator, flags>(allocator, capacity);
}

template <
   is_allocator Allocator, str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard]]
constexpr auto
make_wzstr_vec_reserved(allocator_ref<Allocator> allocator, idx capacity)
   -> maybe<basic_str_vec<wchar_t, str_flags::null_terminated | flags>> {
   basic_str_vec<wchar_t, str_flags::null_terminated | flags> new_string;
   $prop(new_string.reserve(
      allocator, max(capacity, new_string.flags.vec.initial_growth_count)
   ));
   $prop(new_string.resize(allocator, 0u));
   return new_string;
}

template <str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wzstr_vec_reserved(dyn_allocator allocator, idx capacity)
   -> maybe<basic_str_vec<wchar_t, str_flags::null_terminated | flags>> {
   return make_wzstr_vec_reserved<dyn_allocator, flags>(allocator, capacity);
}

template <
   is_allocator Allocator, str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard]]
constexpr auto
make_str_vec_filled(allocator_ref<Allocator> allocator, idx count, char value)
   -> maybe<basic_str_vec<char, flags>> {
   basic_str_vec<char, flags> new_string =
      $prop((make_str_vec_reserved<Allocator, flags>(allocator, count)));
   for (idx i = 0u; i < count; ++i) {
      $prop(new_string.push_back(allocator, value));
   }
   return new_string;
}

template <str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_str_vec_filled(dyn_allocator allocator, idx count, char value)
   -> maybe<basic_str_vec<char, flags>> {
   return make_str_vec_filled<dyn_allocator, flags>(allocator, count, value);
}

template <
   is_allocator Allocator, str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard]]
constexpr auto
make_zstr_vec_filled(allocator_ref<Allocator> allocator, idx count, char value)
   -> maybe<basic_str_vec<char, str_flags::null_terminated | flags>> {
   basic_str_vec<char, str_flags::null_terminated | flags> new_string =
      $prop((make_zstr_vec_reserved<Allocator, flags>(allocator, count)));
   $prop(new_string.resize(allocator, count, value));
   return new_string;
}

template <str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_zstr_vec_filled(dyn_allocator allocator, idx count, char value)
   -> maybe<basic_str_vec<char, str_flags::null_terminated | flags>> {
   return make_zstr_vec_filled<dyn_allocator, flags>(allocator, count, value);
}

template <
   is_allocator Allocator, str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard]]
constexpr auto
make_wstr_vec_filled(
   allocator_ref<Allocator> allocator, idx count, wchar_t value
) -> maybe<basic_str_vec<wchar_t, flags>> {
   basic_str_vec<wchar_t, flags> new_string =
      $prop((make_wstr_vec_reserved<Allocator, flags>(allocator, count)));
   for (idx i = 0u; i < count; ++i) {
      $prop(new_string.push_back(allocator, value));
   }
   return new_string;
}

template <str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wstr_vec_filled(dyn_allocator allocator, idx count, wchar_t value)
   -> maybe<basic_str_vec<wchar_t, flags>> {
   return make_wstr_vec_filled<dyn_allocator, flags>(allocator, count, value);
}

template <
   is_allocator Allocator, str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard]]
constexpr auto
make_wzstr_vec_filled(
   allocator_ref<Allocator> allocator, idx count, wchar_t value
) -> maybe<basic_str_vec<wchar_t, str_flags::null_terminated | flags>> {
   basic_str_vec<wchar_t, str_flags::null_terminated | flags> new_string =
      $prop((make_wzstr_vec_reserved<Allocator, flags>(allocator, count)));
   $prop(new_string.resize(allocator, count, value));
   return new_string;
}

template <str_vec_flags flags = vec_flags::pointer_size_layout>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wzstr_vec_filled(dyn_allocator allocator, idx count, wchar_t value)
   -> maybe<basic_str_vec<wchar_t, str_flags::null_terminated | flags>> {
   return make_wzstr_vec_filled<dyn_allocator, flags>(allocator, count, value);
}

}  // namespace manual

}  // namespace cat
