// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/vec>

#include "./str_span.hpp"

namespace cat {

namespace raii {
template <typename CharT, bool is_null_terminated, is_allocator Allocator>
class basic_str_vec;
}  // namespace raii

inline namespace manual {

template <typename CharT, bool is_null_terminated>
class basic_str_vec;

using str_vec = basic_str_vec<char, false>;
using zstr_vec = basic_str_vec<char, true>;
using wstr_vec = basic_str_vec<wchar_t, false>;
using wzstr_vec = basic_str_vec<wchar_t, true>;

}  // namespace manual

namespace detail {
template <typename CharT, bool is_null_terminated>
constexpr auto
maybe_str_vec_has_value(basic_str_vec<CharT, is_null_terminated> const& value)
   -> bool;

template <typename CharT, bool is_null_terminated>
constexpr auto
maybe_str_vec_nullopt() -> basic_str_vec<CharT, is_null_terminated>;
}  // namespace detail

inline namespace manual {

template <typename CharT, bool null_terminated>
class
   [[clang::preferred_name(str_vec), clang::preferred_name(zstr_vec),
     clang::preferred_name(wstr_vec), clang::preferred_name(wzstr_vec),
     gsl::Owner]]
   basic_str_vec
    : public container_interface<basic_str_vec<CharT, null_terminated>, CharT>,
      public random_access_stepanov_iterable_interface<CharT> {
   template <typename, bool, is_allocator>
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

   template <bool other_is_null_terminated>
   [[nodiscard]]
   constexpr auto
   operator==(basic_str_span<CharT const, other_is_null_terminated> rhs) const
      -> bool {
      return compare_strings(
         this->view(), basic_str_span<CharT const, false>(rhs)
      );
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
      return m_core.size();
   }

   [[nodiscard]]
   constexpr auto
   is_null_terminated() const -> bool {
      if constexpr (null_terminated) {
         return true;
      } else {
         return m_core.size() != 0u
                && m_core[idx(m_core.size() - 1u)] == CharT{'\0'};
      }
   }

   [[nodiscard]]
   constexpr auto
   capacity() const -> idx {
      return m_core.capacity();
   }

   [[nodiscard]]
   constexpr auto
   view() const [[clang::lifetimebound]] -> basic_str_span<CharT const, false> {
      if (m_core.data() == nullptr || m_core.size() == 0u) {
         return {};
      }
      if constexpr (null_terminated) {
         return {m_core.data(), content_size()};
      } else {
         return {m_core.data(), m_core.size()};
      }
   }

   [[nodiscard]]
   constexpr auto
   span() [[clang::lifetimebound]] -> basic_str_span<CharT, null_terminated> {
      if (m_core.data() == nullptr) {
         return {};
      }
      return {m_core.data(), m_core.size()};
   }

   [[nodiscard]]
   constexpr auto
   span() const [[clang::lifetimebound]]
   -> basic_str_span<CharT const, null_terminated> {
      if (m_core.data() == nullptr) {
         return {};
      }
      return {m_core.data(), m_core.size()};
   }

   [[nodiscard]]
   constexpr
   operator basic_str_span<CharT, null_terminated>() [[clang::lifetimebound]] {
      return this->span();
   }

   [[nodiscard]]
   constexpr
   operator basic_str_span<CharT const, null_terminated>() const
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
      -> maybe<void> {
      return m_core.reserve(allocator, minimum_capacity);
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   reserve(dyn_allocator allocator, idx minimum_capacity) -> maybe<void> {
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
      if constexpr (!null_terminated) {
         return m_core.resize(allocator, new_size, value);
      } else {
         if (new_size == 0u) {
            m_core.reset();
            return monostate;
         }

         idx const old_content_size = content_size();
         idx const new_content_size = idx(new_size - 1u);
         $prop(m_core.resize(allocator, new_size, value));
         for (idx i = old_content_size; i < new_content_size; ++i) {
            m_core[i] = value;
         }
         m_core[new_content_size] = CharT{'\0'};
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
      if constexpr (!null_terminated) {
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

   [[nodiscard]]
   constexpr auto
   pop_back() -> maybe<CharT> {
      if constexpr (!null_terminated) {
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
      if constexpr (!null_terminated) {
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
      if constexpr (null_terminated) {
         if !consteval {
            cat::assert(index < content_size());
         }
      }
      this->erase_raw(index, index + 1u);
   }

   constexpr void
   erase(idx start, idx end) {
      if constexpr (null_terminated) {
         if !consteval {
            cat::assert(end <= content_size());
         }
      }
      this->erase_raw(start, end);
   }

   template <is_allocator Allocator>
   [[nodiscard]]
   constexpr auto
   append(
      allocator_ref<Allocator> allocator,
      basic_str_span<CharT const, false> string
   ) -> maybe<void> {
      if constexpr (!null_terminated) {
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
      basic_str_span<CharT const, true> string
   ) -> maybe<void> {
      return append(allocator, basic_str_span<CharT const, false>(string));
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   append(dyn_allocator allocator, basic_str_span<CharT const, false> string)
      -> maybe<void> {
      return append<dyn_allocator>(allocator, string);
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   append(dyn_allocator allocator, basic_str_span<CharT const, true> string)
      -> maybe<void> {
      return append<dyn_allocator>(allocator, string);
   }

   template <is_allocator Allocator, typename Range>
   [[nodiscard]]
   constexpr auto
   append_range(allocator_ref<Allocator> allocator, Range const& range)
      -> maybe<void> {
      for (CharT const value : range) {
         $prop(this->push_back(allocator, value));
      }
      return monostate;
   }

   template <is_allocator Allocator>
   [[nodiscard]]
   constexpr auto
   append_range(
      allocator_ref<Allocator> allocator,
      basic_str_span<CharT const, false> string
   ) -> maybe<void> {
      return append(allocator, string);
   }

   template <is_allocator Allocator>
   [[nodiscard]]
   constexpr auto
   append_range(
      allocator_ref<Allocator> allocator,
      basic_str_span<CharT const, true> string
   ) -> maybe<void> {
      return append(allocator, string);
   }

   template <typename Range>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   append_range(dyn_allocator allocator, Range const& range) -> maybe<void> {
      return append_range<dyn_allocator>(allocator, range);
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
      return cat::basic_vec<CharT>::max_size();
   }

 private:
   [[nodiscard]]
   constexpr auto
   content_size() const -> idx {
      if constexpr (!null_terminated) {
         return m_core.size();
      } else {
         if (m_core.size() == 0u) {
            return 0u;
         }
         return idx(m_core.size() - 1u);
      }
   }

   constexpr void
   erase_raw(idx start, idx end) {
      if !consteval {
         cat::assert(start <= end);
         cat::assert(end <= m_core.size());
      }
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

   template <typename OtherCharT, bool other_is_null_terminated>
   friend constexpr auto
   detail::maybe_str_vec_nullopt()
      -> basic_str_vec<OtherCharT, other_is_null_terminated>;

   cat::basic_vec<CharT, vec_flags::pointer_size_layout> m_core;
};

}  // namespace manual

namespace detail {
template <typename CharT, bool is_null_terminated>
constexpr auto
maybe_str_vec_has_value(basic_str_vec<CharT, is_null_terminated> const& value)
   -> bool {
   return value.data() != nullptr || value.size() == 0u;
}

template <typename CharT, bool is_null_terminated>
constexpr auto
maybe_str_vec_nullopt() -> basic_str_vec<CharT, is_null_terminated> {
   basic_str_vec<CharT, is_null_terminated> value;
   value.m_core = maybe_vec_nullopt<CharT, vec_flags::pointer_size_layout>();
   return value;
}

template <
   typename CharT, bool is_null_terminated, is_allocator Allocator,
   typename First, typename... Rest>
constexpr auto
append_str_vec_parts(
   basic_str_vec<CharT, is_null_terminated>& string,
   allocator_ref<Allocator> allocator, First const& first, Rest const&... rest
) -> maybe<void> {
   basic_str_span<CharT const, false> const view = basic_str_span(first);
   $prop(string.append(allocator, view));
   if constexpr (sizeof...(Rest) == 0u) {
      return monostate;
   } else {
      return append_str_vec_parts(string, allocator, rest...);
   }
}

template <
   typename CharT, bool is_null_terminated, is_allocator Allocator,
   typename... Strings>
[[nodiscard]]
constexpr auto
make_basic_str_vec(
   allocator_ref<Allocator> allocator, Strings const&... strings
) -> maybe<basic_str_vec<CharT, is_null_terminated>> {
   basic_str_vec<CharT, is_null_terminated> new_string;
   idx content_size = 0u;
   ((content_size +=
     basic_str_span<CharT const, false>(basic_str_span(strings)).size()),
    ...);
   $prop(new_string.reserve(
      allocator,
      max(content_size + static_cast<unsigned>(is_null_terminated), 4u)
   ));
   $prop(append_str_vec_parts(new_string, allocator, strings...));
   return new_string;
}

}  // namespace detail

template <typename CharT, bool is_null_terminated>
struct default_compact_trait<basic_str_vec<CharT, is_null_terminated>>
    : identity_trait<compact<
         basic_str_vec<CharT, is_null_terminated>,
         detail::maybe_str_vec_has_value<CharT, is_null_terminated>,
         detail::maybe_str_vec_nullopt<CharT, is_null_terminated>>> {};

inline namespace manual {

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_str_vec(allocator_ref<Allocator> allocator, str_view string)
   -> maybe<str_vec> {
   str_vec new_string;
   $prop(new_string.reserve(allocator, max(string.size(), 4u)));
   $prop(new_string.append(allocator, string));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_str_vec(dyn_allocator allocator, str_view string) -> maybe<str_vec> {
   return make_str_vec<dyn_allocator>(allocator, string);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_zstr_vec(allocator_ref<Allocator> allocator, str_view string)
   -> maybe<zstr_vec> {
   zstr_vec new_string;
   $prop(new_string.reserve(allocator, max(string.size() + 1u, 4u)));
   $prop(new_string.append(allocator, string));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_zstr_vec(dyn_allocator allocator, str_view string) -> maybe<zstr_vec> {
   return make_zstr_vec<dyn_allocator>(allocator, string);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_wstr_vec(allocator_ref<Allocator> allocator, wstr_view string)
   -> maybe<wstr_vec> {
   wstr_vec new_string;
   $prop(new_string.reserve(allocator, max(string.size(), 4u)));
   $prop(new_string.append(allocator, string));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wstr_vec(dyn_allocator allocator, wstr_view string) -> maybe<wstr_vec> {
   return make_wstr_vec<dyn_allocator>(allocator, string);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_wzstr_vec(allocator_ref<Allocator> allocator, wstr_view string)
   -> maybe<wzstr_vec> {
   wzstr_vec new_string;
   $prop(new_string.reserve(allocator, max(string.size() + 1u, 4u)));
   $prop(new_string.append(allocator, string));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wzstr_vec(dyn_allocator allocator, wstr_view string) -> maybe<wzstr_vec> {
   return make_wzstr_vec<dyn_allocator>(allocator, string);
}

template <
   is_allocator Allocator, typename First, typename Second, typename... Strings>
[[nodiscard]]
constexpr auto
make_str_vec(
   allocator_ref<Allocator> allocator, First const& first, Second const& second,
   Strings const&... strings
) -> maybe<str_vec> {
   return detail::make_basic_str_vec<char, false>(
      allocator, first, second, strings...
   );
}

template <typename First, typename Second, typename... Strings>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_str_vec(
   dyn_allocator allocator, First const& first, Second const& second,
   Strings const&... strings
) -> maybe<str_vec> {
   return detail::make_basic_str_vec<char, false, dyn_allocator>(
      allocator, first, second, strings...
   );
}

template <
   is_allocator Allocator, typename First, typename Second, typename... Strings>
[[nodiscard]]
constexpr auto
make_zstr_vec(
   allocator_ref<Allocator> allocator, First const& first, Second const& second,
   Strings const&... strings
) -> maybe<zstr_vec> {
   return detail::make_basic_str_vec<char, true>(
      allocator, first, second, strings...
   );
}

template <typename First, typename Second, typename... Strings>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_zstr_vec(
   dyn_allocator allocator, First const& first, Second const& second,
   Strings const&... strings
) -> maybe<zstr_vec> {
   return detail::make_basic_str_vec<char, true, dyn_allocator>(
      allocator, first, second, strings...
   );
}

template <
   is_allocator Allocator, typename First, typename Second, typename... Strings>
[[nodiscard]]
constexpr auto
make_wstr_vec(
   allocator_ref<Allocator> allocator, First const& first, Second const& second,
   Strings const&... strings
) -> maybe<wstr_vec> {
   return detail::make_basic_str_vec<wchar_t, false>(
      allocator, first, second, strings...
   );
}

template <typename First, typename Second, typename... Strings>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wstr_vec(
   dyn_allocator allocator, First const& first, Second const& second,
   Strings const&... strings
) -> maybe<wstr_vec> {
   return detail::make_basic_str_vec<wchar_t, false, dyn_allocator>(
      allocator, first, second, strings...
   );
}

template <
   is_allocator Allocator, typename First, typename Second, typename... Strings>
[[nodiscard]]
constexpr auto
make_wzstr_vec(
   allocator_ref<Allocator> allocator, First const& first, Second const& second,
   Strings const&... strings
) -> maybe<wzstr_vec> {
   return detail::make_basic_str_vec<wchar_t, true>(
      allocator, first, second, strings...
   );
}

template <typename First, typename Second, typename... Strings>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wzstr_vec(
   dyn_allocator allocator, First const& first, Second const& second,
   Strings const&... strings
) -> maybe<wzstr_vec> {
   return detail::make_basic_str_vec<wchar_t, true, dyn_allocator>(
      allocator, first, second, strings...
   );
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_str_vec(allocator_ref<Allocator> allocator, initializer_list<char> values)
   -> maybe<str_vec> {
   str_vec new_string;
   $prop(new_string.reserve(allocator, max(values.size(), 4u)));
   $prop(new_string.append_range(allocator, values));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_str_vec(dyn_allocator allocator, initializer_list<char> values)
   -> maybe<str_vec> {
   return make_str_vec<dyn_allocator>(allocator, values);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_zstr_vec(allocator_ref<Allocator> allocator, initializer_list<char> values)
   -> maybe<zstr_vec> {
   zstr_vec new_string;
   $prop(new_string.reserve(allocator, max(values.size() + 1u, 4u)));
   $prop(new_string.append_range(allocator, values));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_zstr_vec(dyn_allocator allocator, initializer_list<char> values)
   -> maybe<zstr_vec> {
   return make_zstr_vec<dyn_allocator>(allocator, values);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_wstr_vec(
   allocator_ref<Allocator> allocator, initializer_list<wchar_t> values
) -> maybe<wstr_vec> {
   wstr_vec new_string;
   $prop(new_string.reserve(allocator, max(values.size(), 4u)));
   $prop(new_string.append_range(allocator, values));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wstr_vec(dyn_allocator allocator, initializer_list<wchar_t> values)
   -> maybe<wstr_vec> {
   return make_wstr_vec<dyn_allocator>(allocator, values);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_wzstr_vec(
   allocator_ref<Allocator> allocator, initializer_list<wchar_t> values
) -> maybe<wzstr_vec> {
   wzstr_vec new_string;
   $prop(new_string.reserve(allocator, max(values.size() + 1u, 4u)));
   $prop(new_string.append_range(allocator, values));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wzstr_vec(dyn_allocator allocator, initializer_list<wchar_t> values)
   -> maybe<wzstr_vec> {
   return make_wzstr_vec<dyn_allocator>(allocator, values);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_str_vec_reserved(allocator_ref<Allocator> allocator, idx capacity)
   -> maybe<str_vec> {
   str_vec new_string;
   $prop(new_string.reserve(allocator, max(capacity, 4u)));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_str_vec_reserved(dyn_allocator allocator, idx capacity) -> maybe<str_vec> {
   return make_str_vec_reserved<dyn_allocator>(allocator, capacity);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_zstr_vec_reserved(allocator_ref<Allocator> allocator, idx capacity)
   -> maybe<zstr_vec> {
   zstr_vec new_string;
   $prop(new_string.reserve(allocator, max(capacity, 4u)));
   $prop(new_string.resize(allocator, 1u));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_zstr_vec_reserved(dyn_allocator allocator, idx capacity)
   -> maybe<zstr_vec> {
   return make_zstr_vec_reserved<dyn_allocator>(allocator, capacity);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_wstr_vec_reserved(allocator_ref<Allocator> allocator, idx capacity)
   -> maybe<wstr_vec> {
   wstr_vec new_string;
   $prop(new_string.reserve(allocator, max(capacity, 4u)));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wstr_vec_reserved(dyn_allocator allocator, idx capacity)
   -> maybe<wstr_vec> {
   return make_wstr_vec_reserved<dyn_allocator>(allocator, capacity);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_wzstr_vec_reserved(allocator_ref<Allocator> allocator, idx capacity)
   -> maybe<wzstr_vec> {
   wzstr_vec new_string;
   $prop(new_string.reserve(allocator, max(capacity, 4u)));
   $prop(new_string.resize(allocator, 1u));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wzstr_vec_reserved(dyn_allocator allocator, idx capacity)
   -> maybe<wzstr_vec> {
   return make_wzstr_vec_reserved<dyn_allocator>(allocator, capacity);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_str_vec_filled(allocator_ref<Allocator> allocator, idx count, char value)
   -> maybe<str_vec> {
   str_vec new_string = $prop(make_str_vec_reserved(allocator, count));
   for (idx i = 0u; i < count; ++i) {
      $prop(new_string.push_back(allocator, value));
   }
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_str_vec_filled(dyn_allocator allocator, idx count, char value)
   -> maybe<str_vec> {
   return make_str_vec_filled<dyn_allocator>(allocator, count, value);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_zstr_vec_filled(allocator_ref<Allocator> allocator, idx count, char value)
   -> maybe<zstr_vec> {
   zstr_vec new_string = $prop(make_zstr_vec_reserved(allocator, count));
   $prop(new_string.resize(allocator, count, value));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_zstr_vec_filled(dyn_allocator allocator, idx count, char value)
   -> maybe<zstr_vec> {
   return make_zstr_vec_filled<dyn_allocator>(allocator, count, value);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_wstr_vec_filled(
   allocator_ref<Allocator> allocator, idx count, wchar_t value
) -> maybe<wstr_vec> {
   wstr_vec new_string = $prop(make_wstr_vec_reserved(allocator, count));
   for (idx i = 0u; i < count; ++i) {
      $prop(new_string.push_back(allocator, value));
   }
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wstr_vec_filled(dyn_allocator allocator, idx count, wchar_t value)
   -> maybe<wstr_vec> {
   return make_wstr_vec_filled<dyn_allocator>(allocator, count, value);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_wzstr_vec_filled(
   allocator_ref<Allocator> allocator, idx count, wchar_t value
) -> maybe<wzstr_vec> {
   wzstr_vec new_string = $prop(make_wzstr_vec_reserved(allocator, count));
   $prop(new_string.resize(allocator, count, value));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wzstr_vec_filled(dyn_allocator allocator, idx count, wchar_t value)
   -> maybe<wzstr_vec> {
   return make_wzstr_vec_filled<dyn_allocator>(allocator, count, value);
}

}  // namespace manual

}  // namespace cat
