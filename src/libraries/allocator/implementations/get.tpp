// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// member `get` / `p_get` definitions for `allocator_interface`

namespace cat {

template <typename derived_type>
// Get a non-`const` handle to the data in any memory handle. If that memory
// handle is to a multi-allocation, this returns a `span`.
template <mem T>
[[nodiscard]]
constexpr auto
allocator_interface<derived_type>::get(T& handle) & -> decltype(auto) {
   using allocation_type = T::allocation_type;
   if constexpr (T::is_inline_handle) {
      // Get small-size optimized data:
      if (handle.is_inline()) {
         if constexpr (T::is_multi_handle) {
            return span<allocation_type>{
               __builtin_addressof(handle.get_inline()), handle.size()};
         } else {
            return handle.get_inline();
         }
      }
   }

   // Get non-small-size optimized data:
   if constexpr (T::is_multi_handle) {
      return span<allocation_type>(
         this->self().template access<allocation_type>(handle.get()),
         handle.size());
   } else {
      return *(this->self().template access<allocation_type>(handle.get()));
   }
}

template <typename derived_type>
// Get a `const` handle to the data in any memory handle. If that memory
// handle is to a multi-allocation, this returns a `span`.
template <mem T>
[[nodiscard]]
constexpr auto
allocator_interface<derived_type>::get(T const& memory) & -> decltype(auto) {
   return unconst(this)->get(unconst(memory));
}

template <typename derived_type>
// Get a non-`const` reference to the data in any pointer handle.
template <typename T>
[[nodiscard]]
constexpr auto
allocator_interface<derived_type>::get(T* p_handle) & -> T& {
   return *p_handle;
}

template <typename derived_type>
// Get a `const` reference to the data in any pointer handle.
template <typename T>
[[nodiscard]]
constexpr auto
allocator_interface<derived_type>::get(T const* p_handle) & -> T& {
   return *p_handle;
}

template <typename derived_type>
// Get a pointer to an allocated non-`const` `mem`.
template <mem T>
   requires(derived_type::has_pointer_stability)
[[nodiscard]]
constexpr auto
allocator_interface<derived_type>::p_get(T& memory) -> auto {
   decltype(auto) ref = this->get(memory);
   using handle_type = decltype(this->get(memory));
   if constexpr (is_reference<handle_type>) {
      return __builtin_addressof(ref);
   } else {
      return ref.data();
   }
}

template <typename derived_type>
// Get a `const` pointer to an allocated `mem`.
template <mem T>
   requires(derived_type::has_pointer_stability)
[[nodiscard]]
constexpr auto
allocator_interface<derived_type>::p_get(T const& memory) -> auto {
   decltype(auto) ref = this->get(memory);
   using handle_type = decltype(this->get(memory));
   if constexpr (is_reference<handle_type>) {
      return __builtin_addressof(ref);
   } else {
      return ref.data();
   }
}

}  // namespace cat
