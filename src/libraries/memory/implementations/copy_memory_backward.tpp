// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/memory>

#include "copy_memory_backward_small.tpp"

// `copy_memory_backward()` selects an optimal copy implementation from a
// hierarchy of size ranges. It can do this either at runtime by cascading
// `if`-statements, or potentially at compile-time by attribute `enable_if`
// overloads.

namespace cat {

[[gnu::always_inline]]
constexpr void
copy_memory_backward(
   void const* _Nonnull p_source, void* _Nonnull p_destination, idx bytes
) __attribute__((enable_if(bytes > 0u && bytes < 4u, ""))) {
   if consteval {
      __builtin_memmove(p_destination, p_source, bytes);
   } else {
      detail::copy_memory_backward_small_1_to_3(
         static_cast<byte const* _Nonnull>(p_source),
         static_cast<byte* _Nonnull>(p_destination), bytes
      );
   }
}

[[gnu::always_inline]]
constexpr void
copy_memory_backward(
   void const* _Nonnull p_source, void* _Nonnull p_destination, idx bytes
) __attribute__((enable_if(bytes >= 4u && bytes < 8u, ""))) {
   if consteval {
      __builtin_memmove(p_destination, p_source, bytes);
   } else {
      detail::copy_memory_backward_small_4_to_7(
         static_cast<byte const* _Nonnull>(p_source),
         static_cast<byte* _Nonnull>(p_destination), bytes
      );
   }
}

[[gnu::always_inline]]
constexpr void
copy_memory_backward(
   void const* _Nonnull p_source, void* _Nonnull p_destination, idx bytes
) __attribute__((enable_if(bytes >= 8u && bytes < 16u, ""))) {
   if consteval {
      __builtin_memmove(p_destination, p_source, bytes);
   } else {
      detail::copy_memory_backward_small_8_to_15(
         static_cast<byte const* _Nonnull>(p_source),
         static_cast<byte* _Nonnull>(p_destination), bytes
      );
   }
}

[[gnu::always_inline]]
constexpr void
copy_memory_backward(
   void const* _Nonnull p_source, void* _Nonnull p_destination, idx bytes
) __attribute__((enable_if(bytes >= 16u && bytes <= 32u, ""))) {
   if consteval {
      __builtin_memmove(p_destination, p_source, bytes);
   } else {
      detail::copy_memory_backward_small_16_to_32(
         static_cast<byte const* _Nonnull>(p_source),
         static_cast<byte* _Nonnull>(p_destination), bytes
      );
   }
}

[[gnu::always_inline]]
constexpr void
copy_memory_backward(
   void const* _Nonnull p_source, void* _Nonnull p_destination, idx bytes
) __attribute__((enable_if(bytes >= 33u && bytes <= 64u, ""))) {
   if consteval {
      __builtin_memmove(p_destination, p_source, bytes);
   } else {
      detail::copy_memory_backward_small_33_to_64(
         static_cast<byte const* _Nonnull>(p_source),
         static_cast<byte* _Nonnull>(p_destination), bytes
      );
   }
}

[[gnu::always_inline]]
constexpr void
copy_memory_backward(
   void const* _Nonnull p_source, void* _Nonnull p_destination, idx bytes
) __attribute__((enable_if(bytes >= 65u && bytes <= 127u, ""))) {
   if consteval {
      __builtin_memmove(p_destination, p_source, bytes);
   } else {
      detail::copy_memory_backward_small_65_to_127(
         static_cast<byte const* _Nonnull>(p_source),
         static_cast<byte* _Nonnull>(p_destination), bytes
      );
   }
}

// Unlinke `cat::copy_memory`, this function allows the source and destination
// ranges to overlap.
constexpr void
copy_memory_backward(
   void const* _Nonnull p_source, void* _Nonnull p_destination, idx bytes
) {
   if (bytes == 0u) {
      return;
   }

   if consteval {
      __builtin_memmove(p_destination, p_source, bytes);
   } else {
      // Vectorized implementation for runtime.
      detail::copy_memory_backward_impl(
         static_cast<byte const* _Nonnull>(p_source),
         static_cast<byte* _Nonnull>(p_destination), bytes
      );
   }
}

[[clang::no_builtin("memmove")]]
constexpr void
copy_memory_backward_scalar(
   void const* _Nonnull p_source, void* _Nonnull p_destination, idx bytes
) {
   unsigned char const* _Nonnull const p_src =
      static_cast<unsigned char const* _Nonnull>(p_source);
   unsigned char* _Nonnull const p_dest =
      static_cast<unsigned char* _Nonnull>(p_destination);

   if (p_dest <= p_src) {
      // When the copy is forwards, we wrap `copy_memory_scalar` instead.
      copy_memory_scalar(p_src, p_dest, bytes);
   } else {
#pragma clang loop vectorize(disable)
#pragma unroll 4
      for (iword k = bytes; k > 0u;) {
         k -= 1u;
         // Because `k > 0u` here, we don't check a cast to `idx`.
         idx const subscript = idx{k};
         p_dest[subscript] = p_src[subscript];
      }
   }
}

}  // namespace cat
