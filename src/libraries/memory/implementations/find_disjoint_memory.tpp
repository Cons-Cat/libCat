// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/memory>

namespace cat {

[[clang::no_builtin("memcmp", "bcmp")]]
constexpr auto
find_disjoint_memory_scalar(
   void const* _Nonnull p_lhs, void const* _Nonnull p_rhs, idx bytes
) noexcept -> idx {
   unsigned char const* _Nonnull const p_left =
      static_cast<unsigned char const* _Nonnull>(p_lhs);
   unsigned char const* _Nonnull const p_right =
      static_cast<unsigned char const* _Nonnull>(p_rhs);

#pragma clang loop vectorize(disable)
#pragma unroll 4
   for (idx byte_index = 0u; byte_index < bytes; ++byte_index) {
      if (p_left[byte_index] != p_right[byte_index]) {
         return byte_index;
      }
   }
   return bytes;
}

constexpr auto
find_disjoint_memory(
   void const* _Nonnull p_lhs, void const* _Nonnull p_rhs, idx bytes
) noexcept -> idx {
   if consteval {
      return find_disjoint_memory_scalar(p_lhs, p_rhs, bytes);
   } else {
      if (bytes == 0u) {
         return 0u;
      }

      return detail::find_disjoint_memory_impl(
         static_cast<byte const* _Nonnull>(p_lhs),
         static_cast<byte const* _Nonnull>(p_rhs), bytes
      );
   }
}

}  // namespace cat
