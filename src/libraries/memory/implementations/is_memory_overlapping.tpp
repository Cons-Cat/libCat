// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/span>

namespace cat {

// Disjointness of two half-open byte intervals `[p_first, p_first+first_bytes)`
// and `[p_second, p_second+second_bytes)` as raw addresses (including touching
// at an endpoint as non-overlapping for `memcpy`-class use).
[[nodiscard]]
constexpr auto
is_memory_overlapping(void const* _Nonnull p_first, idx first_bytes,
                      void const* _Nonnull p_second, idx second_bytes) -> bool {
   if (first_bytes == 0u || second_bytes == 0u) {
      return false;
   }
   cat::uintptr const a{const_cast<void* _Nonnull>(p_first)};
   cat::uintptr const b{const_cast<void* _Nonnull>(p_second)};
   cat::uintptr const a_end = a + first_bytes;
   cat::uintptr const b_end = b + second_bytes;
   return b < a_end && a < b_end;
}

}  // namespace cat
