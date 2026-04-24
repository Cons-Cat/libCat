// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/memory>

namespace cat {

// Unlinke `cat::copy_memory`, this function allows the source and destination
// ranges to overlap.
constexpr void
copy_memory_backward(void const* p_source, void* p_destination, idx bytes) {
   if (bytes == 0u) {
      return;
   }

   if consteval {
      __builtin_memmove(p_destination, p_source, bytes);
   } else {
      // Vectorized implementation for runtime.
      detail::copy_memory_backward_impl(p_source, p_destination, bytes);
   }
}

[[clang::no_builtin("memmove")]]
constexpr void
copy_memory_backward_scalar(void const* p_source, void* p_destination,
                            idx bytes) {
   auto const* const p_src = static_cast<unsigned char const*>(p_source);
   auto* const p_dest = static_cast<unsigned char*>(p_destination);

   if (p_dest <= p_src) {
      // When the copy is forwards, we wrap `copy_memory` instead.
      copy_memory(p_src, p_dest, bytes);
   } else {
#pragma clang vectorize(disable) interleave(enable)
      for (iword k = bytes; k > 0u;) {
         k -= 1u;
         // Because `k > 0u` here, we don't check a cast to `idx`.
         idx const subscript = idx{k};
         p_dest[subscript] = p_src[subscript];
      }
   }
}

}  // namespace cat
