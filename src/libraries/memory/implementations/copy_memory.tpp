// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/bit>
#include <cat/debug>

#include "copy_memory_small.tpp"

// `copy_memory()` selects an optimal copy implementation from a hierarchy of
// size ranges. It can do this either at runtime by cascading `if`-statements,
// or potentially at compile-time by attribute `enable_if` overloads.

namespace cat {

[[gnu::always_inline]]
constexpr void
copy_memory(
   void const* _Nonnull __restrict p_source,
   void* _Nonnull __restrict p_destination, idx bytes
) __attribute__((enable_if(bytes > 0u && bytes < 4u, ""))) {
   if consteval {
      __builtin_memcpy(p_destination, p_source, bytes);
   } else {
      detail::copy_memory_small_1_to_3(
         static_cast<byte const* _Nonnull>(p_source),
         static_cast<byte* _Nonnull>(p_destination), bytes
      );
   }
}

[[gnu::always_inline]]
constexpr void
copy_memory(
   void const* _Nonnull __restrict p_source,
   void* _Nonnull __restrict p_destination, idx bytes
) __attribute__((enable_if(bytes >= 4u && bytes < 8u, ""))) {
   if consteval {
      __builtin_memcpy(p_destination, p_source, bytes);
   } else {
      detail::copy_memory_small_4_to_7(
         static_cast<byte const* _Nonnull>(p_source),
         static_cast<byte* _Nonnull>(p_destination), bytes
      );
   }
}

[[gnu::always_inline]]
constexpr void
copy_memory(
   void const* _Nonnull __restrict p_source,
   void* _Nonnull __restrict p_destination, idx bytes
) __attribute__((enable_if(bytes >= 8u && bytes < 16u, ""))) {
   if consteval {
      __builtin_memcpy(p_destination, p_source, bytes);
   } else {
      detail::copy_memory_small_8_to_15(
         static_cast<byte const* _Nonnull>(p_source),
         static_cast<byte* _Nonnull>(p_destination), bytes
      );
   }
}

[[gnu::always_inline]]
constexpr void
copy_memory(
   void const* _Nonnull __restrict p_source,
   void* _Nonnull __restrict p_destination, idx bytes
) __attribute__((enable_if(bytes >= 16u && bytes < 32u, ""))) {
   if consteval {
      __builtin_memcpy(p_destination, p_source, bytes);
   } else {
      detail::copy_memory_small_16_to_31(
         static_cast<byte const* _Nonnull>(p_source),
         static_cast<byte* _Nonnull>(p_destination), bytes
      );
   }
}

[[gnu::always_inline]]
constexpr void
copy_memory(
   void const* _Nonnull __restrict p_source,
   void* _Nonnull __restrict p_destination, idx bytes
) __attribute__((enable_if(bytes >= 32u && bytes < 64u, ""))) {
   if consteval {
      __builtin_memcpy(p_destination, p_source, bytes);
   } else {
      detail::copy_memory_small_32_to_63(
         static_cast<byte const* _Nonnull>(p_source),
         static_cast<byte* _Nonnull>(p_destination), bytes
      );
   }
}

[[gnu::always_inline]]
constexpr void
copy_memory(
   void const* _Nonnull __restrict p_source,
   void* _Nonnull __restrict p_destination, idx bytes
) __attribute__((enable_if(bytes >= 64u && bytes <= 127u, ""))) {
   if consteval {
      __builtin_memcpy(p_destination, p_source, bytes);
   } else {
      detail::copy_memory_small_64_to_127(
         static_cast<byte const* _Nonnull>(p_source),
         static_cast<byte* _Nonnull>(p_destination), bytes
      );
   }
}

// Copy non-overlapping memory ranges.
constexpr void
copy_memory(
   void const* _Nonnull __restrict p_source,
   void* _Nonnull __restrict p_destination, idx bytes
) {
   if (bytes == 0u) {
      return;
   }

   if consteval {
      __builtin_memcpy(p_destination, p_source, bytes);
   } else {
      // Vectorized implementation for runtime.
      detail::copy_memory_impl(
         static_cast<byte const* _Nonnull>(p_source),
         static_cast<byte* _Nonnull>(p_destination), bytes
      );
   }
}

[[clang::no_builtin("memcpy")]]
constexpr void
copy_memory_scalar(
   void const* _Nonnull __restrict p_source,
   void* _Nonnull __restrict p_destination, idx bytes
) {
   unsigned char const* _Nonnull __restrict const p_src =
      static_cast<unsigned char const* _Nonnull>(p_source);
   unsigned char* _Nonnull __restrict const p_dest =
      static_cast<unsigned char* _Nonnull>(p_destination);

#pragma clang loop vectorize(disable)
#pragma unroll 4
   for (idx i = 0u; i < bytes; ++i) {
      p_dest[i] = p_src[i];
   }
}

}  // namespace cat
