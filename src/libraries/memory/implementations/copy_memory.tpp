// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/bit>
#include <cat/debug>

namespace cat {

// Copy non-overlapping memory ranges.
constexpr void
copy_memory(void const* p_source, void* p_destination, uword bytes) {
   if (bytes == 0u) {
      return;
   }

   if consteval {
      __builtin_memcpy(p_destination, p_source, bytes.raw);
   } else {
      // Vectorized implementation for runtime.
      detail::copy_memory_impl(p_source, p_destination, bytes);
   }
}

[[clang::no_builtin("memcpy")]]
constexpr void
copy_memory_scalar(void const* p_source, void* p_destination, uword bytes) {
   unsigned char const* const p_src =
      static_cast<unsigned char const*>(p_source);
   unsigned char* const p_dest = static_cast<unsigned char*>(p_destination);

#pragma clang vectorize(disable)
   for (idx i = 0u; i < bytes; ++i) {
      p_dest[i] = p_src[i];
   }
}

}  // namespace cat
