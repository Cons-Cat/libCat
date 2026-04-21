// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/memory>

namespace cat {

// Unlinke `cat::copy_memory`, this function allows the source and destination
// ranges to overlap.
constexpr void
copy_memory_backward(void const* p_source, void* p_destination, uword bytes) {
   if (bytes == 0u) {
      return;
   }

   if consteval {
      __builtin_memmove(p_destination, p_source, bytes.raw);
   } else {
      // Vectorized implementation for runtime.
      detail::copy_memory_backward_impl(p_source, p_destination, bytes);
   }
}

[[clang::no_builtin("memmove")]]
constexpr void
copy_memory_backward_scalar(void const* p_source, void* p_destination,
                            uword bytes) {
   unsigned char const* const p_src =
      static_cast<unsigned char const*>(p_source);
   unsigned char* const p_dest = static_cast<unsigned char*>(p_destination);

   if (p_dest <= p_src) {
#pragma clang vectorize(disable)
      for (idx i = 0u; i < bytes; ++i) {
         p_dest[i] = p_src[i];
      }
   } else {
#pragma clang vectorize(disable)
      for (uword k = bytes; k > 0u;) {
         k -= 1u;
         idx const i{k};
         p_dest[i] = p_src[i];
      }
   }
}

}  // namespace cat
