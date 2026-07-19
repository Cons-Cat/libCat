// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/allocator_parameters>

namespace cat::detail {

template <typename T>
union vec_niche_storage {
   byte inactive;
   T value;

   constexpr vec_niche_storage() : inactive{} {
   }

   constexpr ~vec_niche_storage() {
   }
};

template <typename T>
inline constexpr vec_niche_storage<T> vec_niche{};

template <typename T, bool pointer_size_layout>
struct vec_bounds;

template <typename T>
struct vec_bounds<T, true> {
   T* _Nullable m_p_data = nullptr;
   idx m_size = 0u;
   idx m_capacity = 0u;

   [[nodiscard]]
   constexpr auto
   data() const -> T* _Nullable {
      return m_p_data;
   }

   [[nodiscard]]
   constexpr auto
   size() const -> idx {
      return m_size;
   }

   [[nodiscard]]
   constexpr auto
   capacity() const -> idx {
      return m_capacity;
   }

   constexpr void
   set_layout(T* _Nullable p_data, idx size, idx capacity) {
      m_p_data = p_data;
      m_size = size;
      m_capacity = capacity;
   }

   constexpr void
   set_size(idx size) {
      m_size = size;
   }

   [[nodiscard]]
   constexpr auto
   has_value() const -> bool {
      return m_p_data != nullptr || m_size == 0u;
   }

   constexpr void
   set_niche() {
      set_layout(nullptr, 1u, 0u);
   }
};

template <typename T>
struct vec_bounds<T, false> {
   T* _Nullable m_p_begin = nullptr;
   T* _Nullable m_p_end = nullptr;
   T const* _Nullable m_p_capacity = nullptr;

   [[nodiscard]]
   constexpr auto
   data() const -> T* _Nullable {
      return m_p_begin;
   }

   [[nodiscard]]
   constexpr auto
   size() const -> idx {
      return m_p_begin == nullptr ? 0u : idx(m_p_end - m_p_begin);
   }

   [[nodiscard]]
   constexpr auto
   capacity() const -> idx {
      return m_p_begin == nullptr ? 0u : idx(m_p_capacity - m_p_begin);
   }

   constexpr void
   set_layout(T* _Nullable p_data, idx size, idx capacity) {
      m_p_begin = p_data;
      m_p_end = p_data == nullptr ? nullptr : p_data + size;
      m_p_capacity = p_data == nullptr ? nullptr : p_data + capacity;
   }

   constexpr void
   set_size(idx size) {
      m_p_end = m_p_begin + size;
   }

   [[nodiscard]]
   constexpr auto
   has_value() const -> bool {
      return m_p_begin != nullptr || m_p_end != nullptr
             || m_p_capacity != __builtin_addressof(vec_niche<T>.value);
   }

   constexpr void
   set_niche() {
      m_p_begin = nullptr;
      m_p_end = nullptr;
      m_p_capacity = __builtin_addressof(vec_niche<T>.value);
   }
};

template <
   typename T, vec_flags flags,
   bool has_inline_storage = (flags.inline_storage_count != 0u)>
struct vec_storage : vec_bounds<T, flags.uses_pointer_size_layout> {
   static constexpr bool is_inline = false;

   constexpr vec_storage() = default;

   constexpr vec_storage(vec_storage&& other)
       : vec_bounds<T, flags.uses_pointer_size_layout>(other) {
      other.set_layout(nullptr, 0u, 0u);
   }

   constexpr auto
   operator=(vec_storage&& other) -> vec_storage& {
      static_cast<vec_bounds<T, flags.uses_pointer_size_layout>&>(*this) =
         other;
      other.set_layout(nullptr, 0u, 0u);
      return *this;
   }

   constexpr void
   disarm_inline() {
   }
};

template <typename T, vec_flags flags>
struct vec_storage<T, flags, true>
    : vec_bounds<T, flags.uses_pointer_size_layout> {
   using handle_type =
      remove_cvref<decltype(declval<allocator_ref<dyn_allocator>&>()
                               .template inline_salloc_multi<
                                  T, flags.inline_storage_count * sizeof(T)>(1u)
                               .value()
                               .first())>;

   static constexpr bool is_inline = true;

   handle_type m_handle{};
   bool m_has_handle = false;

   constexpr vec_storage() = default;

   constexpr vec_storage(vec_storage&& other)
       : vec_bounds<T, flags.uses_pointer_size_layout>(other),
         m_handle(cat::move(other.m_handle)),
         m_has_handle(other.m_has_handle) {
      if (m_has_handle && m_handle.is_inline()) {
         this->set_layout(
            __builtin_addressof(m_handle.get_inline()), other.size(),
            other.capacity()
         );
         other.m_handle.set_inlined(false);
      }
      other.set_layout(nullptr, 0u, 0u);
      other.m_has_handle = false;
   }

   constexpr auto
   operator=(vec_storage&& other) -> vec_storage& {
      if (this == __builtin_addressof(other)) {
         return *this;
      }
      static_cast<vec_bounds<T, flags.uses_pointer_size_layout>&>(*this) =
         other;
      m_handle = cat::move(other.m_handle);
      m_has_handle = other.m_has_handle;
      if (m_has_handle && m_handle.is_inline()) {
         this->set_layout(
            __builtin_addressof(m_handle.get_inline()), other.size(),
            other.capacity()
         );
         other.m_handle.set_inlined(false);
      }
      other.set_layout(nullptr, 0u, 0u);
      other.m_has_handle = false;
      return *this;
   }

   constexpr void
   disarm_inline() {
      if (m_has_handle && m_handle.is_inline()) {
         m_handle.set_inlined(false);
      }
      m_has_handle = false;
   }
};

}  // namespace cat::detail
