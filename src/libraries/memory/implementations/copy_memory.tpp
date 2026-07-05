// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/bit>
#include <cat/debug>

namespace cat {

// Copy non-overlapping memory ranges.
constexpr void
copy_memory(void const* _Nonnull __restrict p_source,
            void* _Nonnull __restrict p_destination, idx bytes) {
   if (bytes == 0u) {
      return;
   }

   if consteval {
      __builtin_memcpy(p_destination, p_source, bytes);
   } else {
      // Vectorized implementation for runtime.
      detail::copy_memory_impl(static_cast<byte const* _Nonnull>(p_source),
                               static_cast<byte* _Nonnull>(p_destination),
                               bytes);
   }
}

[[clang::no_builtin("memcpy")]]
constexpr void
copy_memory_scalar(void const* _Nonnull __restrict p_source,
                   void* _Nonnull __restrict p_destination, idx bytes) {
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
