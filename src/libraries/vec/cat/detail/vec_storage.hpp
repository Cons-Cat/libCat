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

template <typename T>
struct vec_count_extent {
   idx m_count = 0u;

   [[nodiscard]]
   constexpr auto
   get([[maybe_unused]] T const* _Nullable p_data) const -> idx {
      return m_count;
   }

   constexpr void
   set([[maybe_unused]] T const* _Nullable p_data, idx count) {
      m_count = count;
   }
};

template <typename T>
struct vec_pointer_extent {
   T const* _Nullable m_p_end = nullptr;

   [[nodiscard]]
   constexpr auto
   get(T const* _Nullable p_data) const -> idx {
      return p_data == nullptr ? 0u : idx(m_p_end - p_data);
   }

   constexpr void
   set(T const* _Nullable p_data, idx count) {
      m_p_end = p_data == nullptr ? nullptr : p_data + count;
   }
};

template <typename T, bool pointer_size_layout>
using vec_extent =
   conditional<pointer_size_layout, vec_count_extent<T>, vec_pointer_extent<T>>;

template <typename Extent>
struct vec_explicit_size {
   [[no_unique_address]]
   Extent m_extent{};

   template <typename T>
   [[nodiscard]]
   constexpr auto
   get(T const* _Nullable p_data, [[maybe_unused]] Extent const& capacity) const
      -> idx {
      return m_extent.get(p_data);
   }

   template <typename T>
   constexpr void
   set(
      T const* _Nullable p_data, idx size,
      [[maybe_unused]] Extent const& capacity
   ) {
      m_extent.set(p_data, size);
   }
};

struct vec_capacity_size {
   template <typename T, typename Extent>
   [[nodiscard]]
   constexpr auto
   get(T const* _Nullable p_data, Extent const& capacity) const -> idx {
      return capacity.get(p_data);
   }

   template <typename T, typename Extent>
   constexpr void
   set(T const* _Nullable p_data, idx size, Extent const& capacity) {
      cat::assert(size == capacity.get(p_data));
   }
};

template <typename Extent, bool fixed_size>
using vec_size =
   conditional<fixed_size, vec_capacity_size, vec_explicit_size<Extent>>;

template <typename T, bool pointer_size_layout, bool fixed_size>
struct vec_bounds {
   using extent_type = vec_extent<T, pointer_size_layout>;
   using size_type = vec_size<extent_type, fixed_size>;

   T* _Nullable m_p_data = nullptr;
   [[no_unique_address]]
   size_type m_size{};
   [[no_unique_address]]
   extent_type m_capacity{};

   [[nodiscard]]
   constexpr auto
   data() const -> T* _Nullable {
      return m_p_data;
   }

   [[nodiscard]]
   constexpr auto
   size() const -> idx {
      return m_size.get(m_p_data, m_capacity);
   }

   [[nodiscard]]
   constexpr auto
   capacity() const -> idx {
      return m_capacity.get(m_p_data);
   }

   constexpr void
   set_layout(T* _Nullable p_data, idx size, idx capacity) {
      m_p_data = p_data;
      m_capacity.set(p_data, capacity);
      m_size.set(p_data, size, m_capacity);
   }

   constexpr void
   set_size(idx size) {
      m_size.set(m_p_data, size, m_capacity);
   }

   [[nodiscard]]
   constexpr auto
   has_value() const -> bool {
      if constexpr (pointer_size_layout) {
         if constexpr (fixed_size) {
            return m_p_data != nullptr || capacity() == 0u;
         } else {
            return m_p_data != nullptr || size() == 0u;
         }
      } else {
         return m_p_data != nullptr
                || m_capacity.m_p_end
                      != __builtin_addressof(vec_niche<T>.value);
      }
   }

   constexpr void
   set_niche() {
      if constexpr (pointer_size_layout) {
         if constexpr (fixed_size) {
            set_layout(nullptr, 0u, 1u);
         } else {
            set_layout(nullptr, 1u, 0u);
         }
      } else {
         set_layout(nullptr, 0u, 0u);
         m_capacity.m_p_end = __builtin_addressof(vec_niche<T>.value);
      }
   }
};

template <
   typename T, vec_flags flags,
   bool has_inline_storage = (flags.inline_storage_count != 0u)>
struct vec_storage
    : vec_bounds<T, flags.uses_pointer_size_layout, flags.is_fixed_size> {
   static constexpr bool is_inline = false;

   constexpr vec_storage() = default;

   constexpr vec_storage(vec_storage&& other)
       : vec_bounds<T, flags.uses_pointer_size_layout, flags.is_fixed_size>(
            other
         ) {
      other.set_layout(nullptr, 0u, 0u);
   }

   constexpr auto
   operator=(vec_storage&& other) -> vec_storage& {
      static_cast<
         vec_bounds<T, flags.uses_pointer_size_layout, flags.is_fixed_size>&>(
         *this
      ) = other;
      other.set_layout(nullptr, 0u, 0u);
      return *this;
   }

   constexpr void
   disarm_inline() {
   }
};

template <typename T, vec_flags flags>
struct vec_storage<T, flags, true>
    : vec_bounds<T, flags.uses_pointer_size_layout, flags.is_fixed_size> {
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
       : vec_bounds<T, flags.uses_pointer_size_layout, flags.is_fixed_size>(
            other
         ),
         m_handle(move(other.m_handle)),
         m_has_handle(other.m_has_handle) {
      if (m_has_handle && m_handle.is_inline()) {
         this->set_layout(
            m_handle.get_inline_ptr(), other.size(), other.capacity()
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
      static_cast<
         vec_bounds<T, flags.uses_pointer_size_layout, flags.is_fixed_size>&>(
         *this
      ) = other;
      m_handle = move(other.m_handle);
      m_has_handle = other.m_has_handle;
      if (m_has_handle && m_handle.is_inline()) {
         this->set_layout(
            m_handle.get_inline_ptr(), other.size(), other.capacity()
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
