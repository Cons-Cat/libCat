// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/allocator>

// Member `.get()`/`.get_ptr()` definitions for `allocator_interface`.

namespace cat {

template <typename Derived>
// Get a non-`const` handle to the data in any memory handle. If that memory
// handle is to a multi-allocation, this returns a `span`.
template <typename T>
   requires(!is_const<T>)
[[nodiscard]]
constexpr auto
allocator_interface<Derived>::get(
   T& handle [[clang::lifetimebound]]
) & -> decltype(auto) {
   using value_type = T::value_type;
   if constexpr (T::is_inline_handle) {
      // Get small-size optimized data:
      if (handle.is_inline()) {
         if constexpr (T::is_multi_handle) {
            return span<value_type>{
               __builtin_addressof(handle.get_inline()),
               handle.size(),
            };
         } else {
            return handle.get_inline();
         }
      }
   }

   // Get non-small-size optimized data:
   if constexpr (T::is_multi_handle) {
      return span<value_type>(handle.get().p_storage, handle.size());
   } else {
      return *handle.get().p_storage;
   }
}

template <typename Derived>
// Get a `const` handle to the data in any memory handle. If that memory handle
// is to a multi-allocation, this returns a `span`.
template <typename T>
[[nodiscard]]
constexpr auto
allocator_interface<Derived>::get(
   T const& memory [[clang::lifetimebound]]
) & -> decltype(auto) {
   return unconst(this)->get(unconst(memory));
}

template <typename Derived>
// Get a non-`const` reference to the data in any pointer handle.
template <typename T>
[[nodiscard]]
constexpr auto
allocator_interface<Derived>::get(T* _Nonnull p_handle) & -> T& {
   return *p_handle;
}

template <typename Derived>
// Get a `const` reference to the data in any pointer handle.
template <typename T>
[[nodiscard]]
constexpr auto
allocator_interface<Derived>::get(T const* _Nonnull p_handle) & -> T& {
   return *p_handle;
}

template <typename Derived>
// Get a pointer to an allocated non-`const` memory handle.
template <typename T>
[[nodiscard]]
constexpr auto
allocator_interface<Derived>::get_ptr(T& memory) -> auto {
   // Omitting `[[clang::lifetimebound]]` on this parameter avoids Clang 23
   // `-Wreturn-stack-address` false positives on the `cat::span` branch when
   // `get(memory).data()` still refers to storage owned by `memory`.
   using get_result = decltype(this->get(memory));
   if constexpr (is_reference<get_result>) {
      decltype(auto) ref = this->get(memory);
      return __builtin_addressof(ref);
   } else {
      return this->get(memory).data();
   }
}

template <typename Derived>
// Get a `const` pointer to an allocated memory handle.
template <typename T>
[[nodiscard]]
constexpr auto
allocator_interface<Derived>::get_ptr(T const& memory) -> auto {
   // Omitting `[[clang::lifetimebound]]` on this parameter avoids Clang 23
   // `-Wreturn-stack-address` false positives on the `cat::span` branch when
   // `get(memory).data()` still refers to storage owned by `memory`.
   using get_result = decltype(this->get(memory));
   if constexpr (is_reference<get_result>) {
      decltype(auto) ref = this->get(memory);
      return __builtin_addressof(ref);
   } else {
      return this->get(memory).data();
   }
}

}  // namespace cat
