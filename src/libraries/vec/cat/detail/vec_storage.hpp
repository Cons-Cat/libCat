// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/allocator_parameters>

namespace cat::detail {

// Storage for `vec_niche`, which never constructs its `T`.
//
// TODO: Remove the constructor and destructor when Clang implements P3074.
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3074r7.html
template <typename T>
union vec_niche_storage {
   T value;
   // Makes this a literal type when `T` is not one.
   unsigned char _;

   constexpr vec_niche_storage() {
   }

   constexpr ~vec_niche_storage() {
   }
};

// Its address is the pointer layout's `maybe<vec>` niche.
template <typename T>
inline constexpr vec_niche_storage<T> vec_niche{};

// Supporting multiple `vec` member layouts necessitates abstraction of its
// accessors. These are the implementations for each policy:

// Get/set the extent in a head pointer+size layout `vec`.
template <typename T>
struct vec_count_extent {
   uint4 m_count = 0u;

   [[nodiscard]]
   constexpr auto
   get([[maybe_unused]] T const* _Nullable p_data) const -> idx {
      return m_count;
   }

   constexpr void
   set([[maybe_unused]] T const* _Nullable p_data, idx count) {
      m_count = narrow_cast<uint4>(count).assert();
   }
};

// Get/set the extent in a head+tail pointer layout `vec`.
template <typename T>
struct vec_pointer_extent {
   T const* _Nullable m_p_end = nullptr;

   [[nodiscard]]
   constexpr auto
   get(T const* _Nullable p_data) const -> idx {
      // Both are null, the `maybe<vec>` niche, or in one allocation, so this is
      // defined.
      return idx(m_p_end - p_data);
   }

   constexpr void
   set(T const* _Nullable p_data, idx count) {
      // `nullptr + 0` is `nullptr`.
      m_p_end = p_data + count;
   }
};

template <typename T, vec_flags flags>
using vec_extent = conditional<
   flags.uses_pointer_size_layout, vec_count_extent<T>, vec_pointer_extent<T>>;

// Get/set the size of a resizable `vec`, stored apart from its capacity.
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

// Get/set the size of a fixed-size `vec`, which is always its capacity.
struct vec_capacity_size {
   template <typename T, typename Extent>
   [[nodiscard]]
   constexpr auto
   get(T const* _Nullable p_data, Extent const& capacity) const -> idx {
      return capacity.get(p_data);
   }

   template <typename T, typename Extent>
   constexpr void
   set(T const* _Nullable p_data, idx size, Extent const& capacity) noexcept(
      !__has_feature(cxx_exceptions)
   ) {
      assert(size == capacity.get(p_data));
   }
};

template <typename Extent, bool fixed_size>
using vec_size =
   conditional<fixed_size, vec_capacity_size, vec_explicit_size<Extent>>;

// The data pointer, size, and capacity of a `vec` in its configured layout.
template <typename T, vec_flags flags>
struct vec_bounds {
   using extent_type = vec_extent<T, flags>;
   using size_type = vec_size<extent_type, flags.is_fixed_size>;

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

   // Unlike `set_size()`, this does not reload a possibly aliased `m_p_data`.
   constexpr void
   increment_size()
      requires(!flags.is_fixed_size)
   {
      if constexpr (flags.uses_pointer_size_layout) {
         ++m_size.m_extent.m_count;
      } else {
         ++m_size.m_extent.m_p_end;
      }
   }

   // The pointer layout's `maybe<vec>` niche, neither null nor an allocation.
   [[nodiscard]]
   static constexpr auto
   niche_data() -> T* _Nonnull {
      return const_cast<T*>(__builtin_addressof(vec_niche<T>.value));
   }

   // Whether `m_p_data` is an allocation, not null or the `maybe<vec>` niche.
   [[nodiscard]]
   constexpr auto
   has_storage() const -> bool {
      return m_p_data != nullptr && m_p_data != niche_data();
   }

   [[nodiscard]]
   constexpr auto
   has_value() const -> bool {
      if constexpr (flags.uses_pointer_size_layout) {
         if constexpr (flags.is_fixed_size) {
            return m_p_data != nullptr || capacity() == 0u;
         } else {
            return m_p_data != nullptr || size() == 0u;
         }
      } else {
         return m_p_data != niche_data();
      }
   }

   constexpr void
   set_niche() {
      if constexpr (flags.uses_pointer_size_layout) {
         if constexpr (flags.is_fixed_size) {
            set_layout(nullptr, 0u, 1u);
         } else {
            set_layout(nullptr, 1u, 0u);
         }
      } else {
         set_layout(niche_data(), 0u, 0u);
      }
   }
};

// The members of a `vec` without inline storage.
template <
   typename T, vec_flags flags,
   bool has_inline_storage = (flags.container.inline_storage_count != 0u)>
struct vec_storage : vec_bounds<T, flags> {
   static constexpr bool is_inline = false;

   constexpr vec_storage() = default;

   constexpr vec_storage(vec_storage&& other) : vec_bounds<T, flags>(other) {
      other.set_layout(nullptr, 0u, 0u);
   }

   constexpr auto
   operator=(vec_storage&& other) -> vec_storage& {
      static_cast<vec_bounds<T, flags>&>(*this) = other;
      other.set_layout(nullptr, 0u, 0u);
      return *this;
   }

   constexpr void
   disarm_inline() {
   }
};

// The members of a `vec`, plus the allocator handle holding its inline buffer.
template <typename T, vec_flags flags>
struct vec_storage<T, flags, true> : vec_bounds<T, flags> {
   using handle_type = remove_cvref<
      decltype(declval<allocator_ref<allocator_dyn>&>()
                  .template inline_align_salloc_multi_uninit<
                     T, flags.container.inline_storage_count
                           * sizeof(T)>(flags.container.minimum_alignment, 1u)
                  .value()
                  .first())>;

   static constexpr bool is_inline = true;

   // The inline buffer sits at the start of the handle.
   alignas(handle_type) alignas(
      flags.container.minimum_alignment.raw
   ) handle_type m_handle{};
   bool m_has_handle = false;

   constexpr vec_storage() = default;

   constexpr vec_storage(vec_storage&& other)
       : vec_bounds<T, flags>(other),
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
      static_cast<vec_bounds<T, flags>&>(*this) = other;
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
