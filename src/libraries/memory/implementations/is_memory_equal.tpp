// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/memory>

#include "find_disjoint_memory.tpp"

namespace cat {

[[clang::no_builtin("memcmp", "bcmp")]]
constexpr auto
is_memory_equal_scalar(
   void const* _Nonnull p_lhs, void const* _Nonnull p_rhs, idx bytes
) noexcept -> bool {
   return find_disjoint_memory_scalar(p_lhs, p_rhs, bytes) == bytes;
}

constexpr auto
is_memory_equal(
   void const* _Nonnull p_lhs, void const* _Nonnull p_rhs, idx bytes
) noexcept -> bool {
   if consteval {
      return is_memory_equal_scalar(p_lhs, p_rhs, bytes);
   } else {
      if (bytes == 0u) {
         return true;
      }

      return detail::is_memory_equal_impl(
         static_cast<byte const* _Nonnull>(p_lhs),
         static_cast<byte const* _Nonnull>(p_rhs), bytes
      );
   }
}

}  // namespace cat
