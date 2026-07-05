// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/debug>

namespace cat {

[[clang::no_builtin("memcmp")]]
constexpr auto
compare_memory_scalar(void const* _Nonnull p_lhs, void const* _Nonnull p_rhs,
                      idx bytes) -> std::strong_ordering {
   unsigned char const* _Nonnull const p_left =
      static_cast<unsigned char const* _Nonnull>(p_lhs);
   unsigned char const* _Nonnull const p_right =
      static_cast<unsigned char const* _Nonnull>(p_rhs);

#pragma clang loop vectorize(disable)
#pragma unroll 4
   for (idx byte_index = 0u; byte_index < bytes; ++byte_index) {
      if (p_left[byte_index] != p_right[byte_index]) {
         return p_left[byte_index] <=> p_right[byte_index];
      }
   }
   return std::strong_ordering::equal;
}

constexpr auto
compare_memory(void const* _Nonnull p_lhs, void const* _Nonnull p_rhs,
               idx bytes) -> std::strong_ordering {
   if (bytes == 0u) {
      return std::strong_ordering::equal;
   }

   if consteval {
      return compare_memory_scalar(p_lhs, p_rhs, bytes);
   } else {
      return detail::compare_memory_impl(
         static_cast<byte const* _Nonnull>(p_lhs),
         static_cast<byte const* _Nonnull>(p_rhs), bytes);
   }
}

}  // namespace cat
