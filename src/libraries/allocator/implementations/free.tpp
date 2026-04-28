// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/allocator>

// Member `free`/`cfree` definitions for `allocator_interface`.

namespace cat {

template <typename Derived>
// Invalidate any memory handle, invoking its data's destructor.
template <mem T>
constexpr void
allocator_interface<Derived>::free(T const& handle) {
   using allocation_type = T::allocation_type const;

   // If this is not a small-size optimized handle:
   if (!handle.is_inline()) {
      allocation_type const* p_memory;
      if constexpr (T::is_multi_handle) {
         // Get the pointer from a span produced by the allocator.
         p_memory = this->get(handle).data();
      } else {
         // Get the pointer from the allocator.
         p_memory = __builtin_addressof(this->get(handle));
      }

      poison_memory_region(p_memory, sizeof(allocation_type));

      // if constexpr (is_destructible<allocation_type>) {
      for (idx i = 0u; i < handle.size(); ++i) {
         p_memory[i].~allocation_type();
      }
      // }
      this->self().deallocate(static_cast<void const*>(p_memory),
                              handle.raw_size());
   }
   // If this is small-size optimized, storage lives in the handle and is
   // released when the handle is destroyed, so this call is a no-op.
}

template <typename Derived>
// TODO: This needs unit tests.
// Invalidate a pointer handle to a `T`, and call its destructor.
template <typename T>
constexpr void
allocator_interface<Derived>::free(T* p_memory) {
   if constexpr (detail::has_max_allocation_bytes<Derived>) {
      static_assert(sizeof(T) <= Derived::max_allocation_bytes,
                    "This allocation is too large for this allocator!");
   }

   if consteval {
      delete p_memory;
   } else {
      if constexpr (!is_trivially_destructible<T>) {
         p_memory->~T();
      }

      this->self().deallocate(static_cast<void const*>(p_memory), sizeof(T));
      poison_memory_region(p_memory, sizeof(T));
   }
}

template <typename Derived>
// Invalidate a pointer handle to an array of `T`, and call its destructors.
template <typename T>
constexpr void
allocator_interface<Derived>::free_multi(T* p_memory, idx count) {
   if constexpr (detail::has_max_allocation_bytes<Derived>) {
      static_assert(sizeof(T) <= Derived::max_allocation_bytes,
                    "This allocation is too large for this allocator!");
      assert((count * sizeof(T)) <= Derived::max_allocation_bytes,
             "This allocation is too large for this allocator!");
   }

   if consteval {
      // if (count > 0) {
      delete[] p_memory;
      // }
   } else {
      if constexpr (!is_trivially_destructible<T>) {
         for (idx i = 0u; i < count; ++i) {
            p_memory[i].~T();
         }
      }
      this->self().deallocate(static_cast<void const*>(p_memory),
                              count * sizeof(T));

      poison_memory_region(p_memory, sizeof(T) * count);
   }
}

template <typename Derived>
template <mem T>
constexpr void
allocator_interface<Derived>::cfree(T& handle) {
   using allocation_type = T::allocation_type const;

   if (!handle.is_inline()) {
      allocation_type const* p_memory;
      if constexpr (T::is_multi_handle) {
         p_memory = this->get(handle).data();
      } else {
         p_memory = __builtin_addressof(this->get(handle));
      }

      poison_memory_region(p_memory, sizeof(allocation_type));

      for (idx i = 0u; i < handle.size(); ++i) {
         p_memory[i].~allocation_type();
      }
      zero_memory_explicit(
         static_cast<void*>(const_cast<allocation_type*>(p_memory)),
         handle.raw_size());
      this->self().deallocate(static_cast<void const*>(p_memory),
                              handle.raw_size());
   } else {
      zero_memory_explicit(__builtin_addressof(this->get(handle)),
                           handle.raw_size());
   }
}

template <typename Derived>
template <typename T>
constexpr void
allocator_interface<Derived>::cfree(T* p_memory) {
   if constexpr (detail::has_max_allocation_bytes<Derived>) {
      static_assert(sizeof(T) <= Derived::max_allocation_bytes,
                    "This allocation is too large for this allocator!");
   }

   if consteval {
      if constexpr (!is_trivially_destructible<T>) {
         p_memory->~T();
      }
      zero_memory_explicit(p_memory, sizeof(T));
      delete p_memory;
   } else {
      if constexpr (!is_trivially_destructible<T>) {
         p_memory->~T();
      }
      zero_memory_explicit(p_memory, sizeof(T));
      this->self().deallocate(static_cast<void const*>(p_memory), sizeof(T));
      poison_memory_region(p_memory, sizeof(T));
   }
}

template <typename Derived>
template <typename T>
constexpr void
allocator_interface<Derived>::cfree_multi(T* p_memory, idx count) {
   if constexpr (detail::has_max_allocation_bytes<Derived>) {
      static_assert(sizeof(T) <= Derived::max_allocation_bytes,
                    "This allocation is too large for this allocator!");
      assert((count * sizeof(T)) <= Derived::max_allocation_bytes,
             "This allocation is too large for this allocator!");
   }

   if consteval {
      if constexpr (!is_trivially_destructible<T>) {
         for (idx i = 0u; i < count; ++i) {
            p_memory[i].~T();
         }
      }
      zero_memory_explicit(p_memory, static_cast<uword>(count * sizeof(T)));
      delete[] p_memory;
   } else {
      if constexpr (!is_trivially_destructible<T>) {
         for (idx i = 0u; i < count; ++i) {
            p_memory[i].~T();
         }
      }
      zero_memory_explicit(p_memory, static_cast<uword>(count * sizeof(T)));
      this->self().deallocate(static_cast<void const*>(p_memory),
                              count * sizeof(T));
      poison_memory_region(p_memory, sizeof(T) * count);
   }
}

template <typename Derived>
template <typename T>
constexpr void
allocator_interface<Derived>::free(span<T> handle) {
   this->free_multi(handle.data(), handle.size());
}

template <typename Derived>
template <typename T>
constexpr void
allocator_interface<Derived>::cfree(span<T> handle) {
   this->cfree_multi(handle.data(), handle.size());
}

}  // namespace cat
