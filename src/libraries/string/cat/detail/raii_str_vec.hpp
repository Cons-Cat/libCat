// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/null_allocator>

#include "str_vec.hpp"

namespace cat::raii {

template <
   typename CharT, bool is_null_terminated,
   is_allocator Allocator = dyn_allocator>
class basic_str_vec;

template <is_allocator Allocator = dyn_allocator>
using str_vec = basic_str_vec<char, false, Allocator>;

template <is_allocator Allocator = dyn_allocator>
using zstr_vec = basic_str_vec<char, true, Allocator>;

template <is_allocator Allocator = dyn_allocator>
using wstr_vec = basic_str_vec<wchar_t, false, Allocator>;

template <is_allocator Allocator = dyn_allocator>
using wzstr_vec = basic_str_vec<wchar_t, true, Allocator>;

}  // namespace cat::raii

namespace cat {

template <typename CharT, bool null_terminated, is_allocator Allocator>
basic_str_span(raii::basic_str_vec<CharT, null_terminated, Allocator>&)
   -> basic_str_span<CharT, null_terminated>;

template <typename CharT, bool null_terminated, is_allocator Allocator>
basic_str_span(raii::basic_str_vec<CharT, null_terminated, Allocator> const&)
   -> basic_str_span<CharT const, null_terminated>;

}  // namespace cat

namespace cat::detail {
template <typename CharT, bool is_null_terminated>
constexpr auto
raii_str_vec_has_value(
   raii::basic_str_vec<CharT, is_null_terminated, dyn_allocator> const& value
) -> bool;

template <typename CharT, bool is_null_terminated>
constexpr auto
raii_str_vec_nullopt()
   -> raii::basic_str_vec<CharT, is_null_terminated, dyn_allocator>;
}  // namespace cat::detail

namespace cat::raii {

template <typename CharT, bool null_terminated, is_allocator Allocator>
class
   [[clang::preferred_name(str_vec<Allocator>),
     clang::preferred_name(zstr_vec<Allocator>),
     clang::preferred_name(wstr_vec<Allocator>),
     clang::preferred_name(wzstr_vec<Allocator>), gsl::Owner]]
   basic_str_vec : public container_interface<
                      basic_str_vec<CharT, null_terminated, Allocator>, CharT>,
                   public random_access_stepanov_iterable_interface<CharT> {
   template <typename, bool, is_allocator>
   friend class basic_str_vec;

 public:
   constexpr basic_str_vec() =
      delete ("`cat::raii::basic_str_vec` cannot be created without an "
              "allocator.");

   constexpr basic_str_vec(
      allocator_ref<Allocator> allocator [[clang::lifetimebound]]
   )
       : m_core(), m_allocator(allocator) {
   }

   constexpr basic_str_vec(basic_str_vec&&) = default;

   constexpr basic_str_vec(basic_str_vec const&) =
      delete ("Implicit copying of `cat::raii::basic_str_vec` is forbidden. "
              "Call `.clone()` or move instead!");

   auto
   operator=(basic_str_vec const&) -> basic_str_vec& =
      delete ("Implicit copying of `cat::raii::basic_str_vec` is forbidden. "
              "Call `.clone()` or move instead!");

   [[clang::reinitializes]]
   constexpr auto
   operator=(basic_str_vec&& other) -> basic_str_vec& {
      if (this == __builtin_addressof(other)) {
         return *this;
      }

      m_core.free(m_allocator);
      if (m_allocator == other.m_allocator) {
         m_core = cat::move(other.m_core);
      } else {
         if !consteval {
            cat::assert(
               false, "raii::basic_str_vec move-assign across non-equal "
                      "allocators is not yet implemented."
            );
         }
      }
      return *this;
   }

   constexpr ~basic_str_vec() {
      reset();
   }

   [[nodiscard]]
   constexpr auto
   operator==(basic_str_vec const& rhs) const -> bool {
      return m_core == rhs.m_core;
   }

   template <bool other_is_null_terminated>
   [[nodiscard]]
   constexpr auto
   operator==(basic_str_span<CharT const, other_is_null_terminated> rhs) const
      -> bool {
      return m_core == rhs;
   }

   constexpr void
   swap(basic_str_vec& other) {
      if !consteval {
         cat::assert(m_allocator == other.m_allocator);
      }
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
      return m_core.is_null_terminated();
   }

   [[nodiscard]]
   constexpr auto
   capacity() const -> idx {
      return m_core.capacity();
   }

   [[nodiscard]]
   constexpr auto
   view() const [[clang::lifetimebound]] -> basic_str_span<CharT const, false> {
      return m_core.view();
   }

   [[nodiscard]]
   constexpr auto
   span() [[clang::lifetimebound]] -> basic_str_span<CharT, null_terminated> {
      return m_core.span();
   }

   [[nodiscard]]
   constexpr auto
   span() const [[clang::lifetimebound]]
   -> basic_str_span<CharT const, null_terminated> {
      return m_core.span();
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

   [[clang::reinitializes]]
   constexpr void
   free() {
      m_core.free(m_allocator);
   }

   [[clang::reinitializes]]
   constexpr void
   cfree() {
      m_core.cfree(m_allocator);
   }

   [[clang::reinitializes]]
   constexpr void
   reset() {
      this->free();
   }

   [[nodiscard]]
   constexpr auto
   reserve(idx minimum_capacity) -> maybe<void> {
      return m_core.reserve(m_allocator, minimum_capacity);
   }

   [[nodiscard]]
   constexpr auto
   resize(idx new_size) -> maybe<void> {
      return m_core.resize(m_allocator, new_size);
   }

   [[nodiscard]]
   constexpr auto
   resize(idx new_size, CharT value) -> maybe<void> {
      return m_core.resize(m_allocator, new_size, value);
   }

   [[nodiscard]]
   constexpr auto
   shrink_to_fit() -> maybe<void> {
      return m_core.shrink_to_fit(m_allocator);
   }

   [[nodiscard]]
   constexpr auto
   push_back(CharT value) -> maybe<void> {
      return m_core.push_back(m_allocator, value);
   }

   [[nodiscard]]
   constexpr auto
   pop_back() -> maybe<CharT> {
      return m_core.pop_back();
   }

   [[clang::reinitializes]]
   constexpr void
   clear() {
      m_core.clear();
   }

   constexpr void
   erase(idx index) {
      m_core.erase(index);
   }

   constexpr void
   erase(idx start, idx end) {
      m_core.erase(start, end);
   }

   [[nodiscard]]
   constexpr auto
   append(basic_str_span<CharT const, false> string) -> maybe<void> {
      return m_core.append(m_allocator, string);
   }

   [[nodiscard]]
   constexpr auto
   append(basic_str_span<CharT const, true> string) -> maybe<void> {
      return m_core.append(m_allocator, string);
   }

   template <typename Range>
   [[nodiscard]]
   constexpr auto
   append_range(Range const& range) -> maybe<void> {
      return m_core.append_range(m_allocator, range);
   }

   [[nodiscard]]
   static constexpr auto
   max_size() -> idx {
      return cat::basic_str_vec<CharT, null_terminated>::max_size();
   }

   [[nodiscard]]
   constexpr auto
   clone() const& -> maybe<basic_str_vec> {
      return clone(m_allocator);
   }

   template <is_allocator NewAllocator>
   [[nodiscard]]
   constexpr auto
   clone(
      allocator_ref<NewAllocator> allocator
   ) const& -> maybe<basic_str_vec<CharT, null_terminated, NewAllocator>> {
      basic_str_vec<CharT, null_terminated, NewAllocator> new_string(allocator);
      new_string.m_core = $prop(m_core.clone(allocator));
      return new_string;
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   clone(
      dyn_allocator allocator
   ) const& -> maybe<basic_str_vec<CharT, null_terminated, dyn_allocator>> {
      basic_str_vec<CharT, null_terminated, dyn_allocator> new_string(
         allocator
      );
      new_string.m_core = $prop(m_core.clone(allocator));
      return new_string;
   }

   [[nodiscard]]
   constexpr auto
   allocator(this auto&& self) -> allocator_ref<Allocator> {
      return self.m_allocator;
   }

   [[nodiscard, clang::reinitializes]]
   constexpr auto
   release() -> manual::basic_str_vec<CharT, null_terminated> {
      return cat::move(m_core);
   }

 private:
   template <typename OtherCharT, bool other_is_null_terminated>
   friend constexpr auto
   detail::raii_str_vec_nullopt()
      -> basic_str_vec<OtherCharT, other_is_null_terminated, dyn_allocator>;

   manual::basic_str_vec<CharT, null_terminated> m_core;
   [[no_unique_address]]
   allocator_ref<Allocator> m_allocator;
};

}  // namespace cat::raii

namespace cat::detail {

template <typename String, typename CharT, typename First, typename... Rest>
constexpr auto
append_raii_str_vec_parts(
   String& string, First const& first, Rest const&... rest
) -> maybe<void> {
   basic_str_span<CharT const, false> const view = basic_str_span(first);
   $prop(string.append(view));
   if constexpr (sizeof...(Rest) == 0u) {
      return monostate;
   } else {
      return append_raii_str_vec_parts<String, CharT>(string, rest...);
   }
}

template <
   typename CharT, bool is_null_terminated, is_allocator Allocator,
   typename... Strings>
[[nodiscard]]
constexpr auto
make_raii_str_vec(allocator_ref<Allocator> allocator, Strings const&... strings)
   -> maybe<raii::basic_str_vec<CharT, is_null_terminated, Allocator>> {
   raii::basic_str_vec<CharT, is_null_terminated, Allocator> new_string(
      allocator
   );
   idx content_size = 0u;
   ((content_size +=
     basic_str_span<CharT const, false>(basic_str_span(strings)).size()),
    ...);
   $prop(new_string.reserve(
      max(content_size + static_cast<unsigned>(is_null_terminated), 4u)
   ));
   auto appended = append_raii_str_vec_parts<
      raii::basic_str_vec<CharT, is_null_terminated, Allocator>, CharT>(
      new_string, strings...
   );
   $prop(appended);
   return new_string;
}

template <typename CharT, bool is_null_terminated>
constexpr auto
raii_str_vec_has_value(
   raii::basic_str_vec<CharT, is_null_terminated, dyn_allocator> const& value
) -> bool {
   return value.data() != nullptr || value.size() == 0u;
}

inline constexpr null_allocator raii_str_vec_niche_null_allocator{};

template <typename CharT, bool is_null_terminated>
constexpr auto
raii_str_vec_nullopt()
   -> raii::basic_str_vec<CharT, is_null_terminated, dyn_allocator> {
   raii::basic_str_vec<CharT, is_null_terminated, dyn_allocator> value(
      allocator_ref<dyn_allocator>(dyn_allocator(
         const_cast<null_allocator&>(raii_str_vec_niche_null_allocator)
      ))
   );
   value.m_core = maybe_str_vec_nullopt<CharT, is_null_terminated>();
   return value;
}

}  // namespace cat::detail

namespace cat {

template <typename CharT, bool is_null_terminated>
struct default_compact_trait<
   raii::basic_str_vec<CharT, is_null_terminated, dyn_allocator>>
    : identity_trait<compact<
         raii::basic_str_vec<CharT, is_null_terminated, dyn_allocator>,
         detail::raii_str_vec_has_value<CharT, is_null_terminated>,
         detail::raii_str_vec_nullopt<CharT, is_null_terminated>>> {};

}  // namespace cat

namespace cat::raii {

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_str_vec(allocator_ref<Allocator> allocator)
   -> maybe<raii::str_vec<Allocator>> {
   return raii::str_vec<Allocator>(allocator);
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_str_vec(dyn_allocator allocator) -> maybe<raii::str_vec<dyn_allocator>> {
   return raii::make_str_vec<dyn_allocator>(allocator);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_zstr_vec(allocator_ref<Allocator> allocator)
   -> maybe<raii::zstr_vec<Allocator>> {
   raii::zstr_vec<Allocator> new_string(allocator);
   $prop(new_string.resize(1u));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_zstr_vec(dyn_allocator allocator) -> maybe<raii::zstr_vec<dyn_allocator>> {
   return raii::make_zstr_vec<dyn_allocator>(allocator);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_wstr_vec(allocator_ref<Allocator> allocator)
   -> maybe<raii::wstr_vec<Allocator>> {
   return raii::wstr_vec<Allocator>(allocator);
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wstr_vec(dyn_allocator allocator) -> maybe<raii::wstr_vec<dyn_allocator>> {
   return raii::make_wstr_vec<dyn_allocator>(allocator);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_wzstr_vec(allocator_ref<Allocator> allocator)
   -> maybe<raii::wzstr_vec<Allocator>> {
   raii::wzstr_vec<Allocator> new_string(allocator);
   $prop(new_string.resize(1u));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wzstr_vec(dyn_allocator allocator)
   -> maybe<raii::wzstr_vec<dyn_allocator>> {
   return raii::make_wzstr_vec<dyn_allocator>(allocator);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_str_vec(allocator_ref<Allocator> allocator, str_view string)
   -> maybe<raii::str_vec<Allocator>> {
   raii::str_vec<Allocator> new_string(allocator);
   $prop(new_string.reserve(max(string.size(), 4u)));
   $prop(new_string.append(string));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_str_vec(dyn_allocator allocator, str_view string)
   -> maybe<raii::str_vec<dyn_allocator>> {
   return raii::make_str_vec<dyn_allocator>(allocator, string);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_zstr_vec(allocator_ref<Allocator> allocator, str_view string)
   -> maybe<raii::zstr_vec<Allocator>> {
   raii::zstr_vec<Allocator> new_string(allocator);
   $prop(new_string.reserve(max(string.size() + 1u, 4u)));
   $prop(new_string.append(string));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_zstr_vec(dyn_allocator allocator, str_view string)
   -> maybe<raii::zstr_vec<dyn_allocator>> {
   return raii::make_zstr_vec<dyn_allocator>(allocator, string);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_wstr_vec(allocator_ref<Allocator> allocator, wstr_view string)
   -> maybe<raii::wstr_vec<Allocator>> {
   raii::wstr_vec<Allocator> new_string(allocator);
   $prop(new_string.reserve(max(string.size(), 4u)));
   $prop(new_string.append(string));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wstr_vec(dyn_allocator allocator, wstr_view string)
   -> maybe<raii::wstr_vec<dyn_allocator>> {
   return raii::make_wstr_vec<dyn_allocator>(allocator, string);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_wzstr_vec(allocator_ref<Allocator> allocator, wstr_view string)
   -> maybe<raii::wzstr_vec<Allocator>> {
   raii::wzstr_vec<Allocator> new_string(allocator);
   $prop(new_string.reserve(max(string.size() + 1u, 4u)));
   $prop(new_string.append(string));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wzstr_vec(dyn_allocator allocator, wstr_view string)
   -> maybe<raii::wzstr_vec<dyn_allocator>> {
   return raii::make_wzstr_vec<dyn_allocator>(allocator, string);
}

template <
   is_allocator Allocator, typename First, typename Second, typename... Strings>
[[nodiscard]]
constexpr auto
make_str_vec(
   allocator_ref<Allocator> allocator, First const& first, Second const& second,
   Strings const&... strings
) -> maybe<raii::str_vec<Allocator>> {
   return detail::make_raii_str_vec<char, false>(
      allocator, first, second, strings...
   );
}

template <typename First, typename Second, typename... Strings>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_str_vec(
   dyn_allocator allocator, First const& first, Second const& second,
   Strings const&... strings
) -> maybe<raii::str_vec<dyn_allocator>> {
   return raii::make_str_vec<dyn_allocator>(
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
) -> maybe<raii::zstr_vec<Allocator>> {
   return detail::make_raii_str_vec<char, true>(
      allocator, first, second, strings...
   );
}

template <typename First, typename Second, typename... Strings>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_zstr_vec(
   dyn_allocator allocator, First const& first, Second const& second,
   Strings const&... strings
) -> maybe<raii::zstr_vec<dyn_allocator>> {
   return raii::make_zstr_vec<dyn_allocator>(
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
) -> maybe<raii::wstr_vec<Allocator>> {
   return detail::make_raii_str_vec<wchar_t, false>(
      allocator, first, second, strings...
   );
}

template <typename First, typename Second, typename... Strings>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wstr_vec(
   dyn_allocator allocator, First const& first, Second const& second,
   Strings const&... strings
) -> maybe<raii::wstr_vec<dyn_allocator>> {
   return raii::make_wstr_vec<dyn_allocator>(
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
) -> maybe<raii::wzstr_vec<Allocator>> {
   return detail::make_raii_str_vec<wchar_t, true>(
      allocator, first, second, strings...
   );
}

template <typename First, typename Second, typename... Strings>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wzstr_vec(
   dyn_allocator allocator, First const& first, Second const& second,
   Strings const&... strings
) -> maybe<raii::wzstr_vec<dyn_allocator>> {
   return raii::make_wzstr_vec<dyn_allocator>(
      allocator, first, second, strings...
   );
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_str_vec(allocator_ref<Allocator> allocator, initializer_list<char> values)
   -> maybe<raii::str_vec<Allocator>> {
   raii::str_vec<Allocator> new_string(allocator);
   $prop(new_string.reserve(max(values.size(), 4u)));
   $prop(new_string.append_range(values));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_str_vec(dyn_allocator allocator, initializer_list<char> values)
   -> maybe<raii::str_vec<dyn_allocator>> {
   return raii::make_str_vec<dyn_allocator>(allocator, values);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_zstr_vec(allocator_ref<Allocator> allocator, initializer_list<char> values)
   -> maybe<raii::zstr_vec<Allocator>> {
   raii::zstr_vec<Allocator> new_string(allocator);
   $prop(new_string.reserve(max(values.size() + 1u, 4u)));
   $prop(new_string.append_range(values));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_zstr_vec(dyn_allocator allocator, initializer_list<char> values)
   -> maybe<raii::zstr_vec<dyn_allocator>> {
   return raii::make_zstr_vec<dyn_allocator>(allocator, values);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_wstr_vec(
   allocator_ref<Allocator> allocator, initializer_list<wchar_t> values
) -> maybe<raii::wstr_vec<Allocator>> {
   raii::wstr_vec<Allocator> new_string(allocator);
   $prop(new_string.reserve(max(values.size(), 4u)));
   $prop(new_string.append_range(values));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wstr_vec(dyn_allocator allocator, initializer_list<wchar_t> values)
   -> maybe<raii::wstr_vec<dyn_allocator>> {
   return raii::make_wstr_vec<dyn_allocator>(allocator, values);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_wzstr_vec(
   allocator_ref<Allocator> allocator, initializer_list<wchar_t> values
) -> maybe<raii::wzstr_vec<Allocator>> {
   raii::wzstr_vec<Allocator> new_string(allocator);
   $prop(new_string.reserve(max(values.size() + 1u, 4u)));
   $prop(new_string.append_range(values));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wzstr_vec(dyn_allocator allocator, initializer_list<wchar_t> values)
   -> maybe<raii::wzstr_vec<dyn_allocator>> {
   return raii::make_wzstr_vec<dyn_allocator>(allocator, values);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_str_vec_reserved(allocator_ref<Allocator> allocator, idx capacity)
   -> maybe<raii::str_vec<Allocator>> {
   raii::str_vec<Allocator> new_string(allocator);
   $prop(new_string.reserve(max(capacity, 4u)));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_str_vec_reserved(dyn_allocator allocator, idx capacity)
   -> maybe<raii::str_vec<dyn_allocator>> {
   return raii::make_str_vec_reserved<dyn_allocator>(allocator, capacity);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_zstr_vec_reserved(allocator_ref<Allocator> allocator, idx capacity)
   -> maybe<raii::zstr_vec<Allocator>> {
   raii::zstr_vec<Allocator> new_string(allocator);
   $prop(new_string.reserve(max(capacity, 4u)));
   $prop(new_string.resize(1u));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_zstr_vec_reserved(dyn_allocator allocator, idx capacity)
   -> maybe<raii::zstr_vec<dyn_allocator>> {
   return raii::make_zstr_vec_reserved<dyn_allocator>(allocator, capacity);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_wstr_vec_reserved(allocator_ref<Allocator> allocator, idx capacity)
   -> maybe<raii::wstr_vec<Allocator>> {
   raii::wstr_vec<Allocator> new_string(allocator);
   $prop(new_string.reserve(max(capacity, 4u)));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wstr_vec_reserved(dyn_allocator allocator, idx capacity)
   -> maybe<raii::wstr_vec<dyn_allocator>> {
   return raii::make_wstr_vec_reserved<dyn_allocator>(allocator, capacity);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_wzstr_vec_reserved(allocator_ref<Allocator> allocator, idx capacity)
   -> maybe<raii::wzstr_vec<Allocator>> {
   raii::wzstr_vec<Allocator> new_string(allocator);
   $prop(new_string.reserve(max(capacity, 4u)));
   $prop(new_string.resize(1u));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wzstr_vec_reserved(dyn_allocator allocator, idx capacity)
   -> maybe<raii::wzstr_vec<dyn_allocator>> {
   return raii::make_wzstr_vec_reserved<dyn_allocator>(allocator, capacity);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_str_vec_filled(allocator_ref<Allocator> allocator, idx count, char value)
   -> maybe<raii::str_vec<Allocator>> {
   raii::str_vec<Allocator> new_string =
      $prop(raii::make_str_vec_reserved(allocator, count));
   for (idx i = 0u; i < count; ++i) {
      $prop(new_string.push_back(value));
   }
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_str_vec_filled(dyn_allocator allocator, idx count, char value)
   -> maybe<raii::str_vec<dyn_allocator>> {
   return raii::make_str_vec_filled<dyn_allocator>(allocator, count, value);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_zstr_vec_filled(allocator_ref<Allocator> allocator, idx count, char value)
   -> maybe<raii::zstr_vec<Allocator>> {
   raii::zstr_vec<Allocator> new_string =
      $prop(raii::make_zstr_vec_reserved(allocator, count));
   $prop(new_string.resize(count, value));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_zstr_vec_filled(dyn_allocator allocator, idx count, char value)
   -> maybe<raii::zstr_vec<dyn_allocator>> {
   return raii::make_zstr_vec_filled<dyn_allocator>(allocator, count, value);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_wstr_vec_filled(
   allocator_ref<Allocator> allocator, idx count, wchar_t value
) -> maybe<raii::wstr_vec<Allocator>> {
   raii::wstr_vec<Allocator> new_string =
      $prop(raii::make_wstr_vec_reserved(allocator, count));
   for (idx i = 0u; i < count; ++i) {
      $prop(new_string.push_back(value));
   }
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wstr_vec_filled(dyn_allocator allocator, idx count, wchar_t value)
   -> maybe<raii::wstr_vec<dyn_allocator>> {
   return raii::make_wstr_vec_filled<dyn_allocator>(allocator, count, value);
}

template <is_allocator Allocator>
[[nodiscard]]
constexpr auto
make_wzstr_vec_filled(
   allocator_ref<Allocator> allocator, idx count, wchar_t value
) -> maybe<raii::wzstr_vec<Allocator>> {
   raii::wzstr_vec<Allocator> new_string =
      $prop(raii::make_wzstr_vec_reserved(allocator, count));
   $prop(new_string.resize(count, value));
   return new_string;
}

[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
make_wzstr_vec_filled(dyn_allocator allocator, idx count, wchar_t value)
   -> maybe<raii::wzstr_vec<dyn_allocator>> {
   return raii::make_wzstr_vec_filled<dyn_allocator>(allocator, count, value);
}

}  // namespace cat::raii
