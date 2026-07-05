// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/bit>
#include <cat/simd>

#include "copy_memory_backward_medium.tpp"

namespace cat::detail {

template <typename Vector>
void
copy_memory_backward_large(byte const* _Nonnull p_source,
                           byte* _Nonnull p_destination, idx bytes) {
   char const* _Nonnull const p_src = reinterpret_cast<char const*>(p_source);
   char* _Nonnull const p_dest = reinterpret_cast<char*>(p_destination);

   constexpr idx l3_cache_size = 2_umi;
   constexpr idx step_size = sizeof(Vector) * 8u;

   iword tail_bytes = bytes;
   Vector vectors[8];

   if (tail_bytes <= l3_cache_size) {
      while (tail_bytes >= step_size) {
         tail_bytes -= step_size;

#pragma unroll 8
         for (idx vector_index = 0u; vector_index < 8u; ++vector_index) {
            char const* _Nonnull const p_byte =
               p_src + tail_bytes + (vector_index * sizeof(Vector));
            vectors[vector_index].load_unaligned(p_byte);
         }

#pragma unroll 8
         for (idx vector_index = 0u; vector_index < 8u; ++vector_index) {
            vectors[vector_index].store_unaligned(
               p_dest + tail_bytes + (vector_index * sizeof(Vector)));
         }
      }
   } else {
      while (tail_bytes > step_size
             && (cat::uintptr<char>{p_dest + tail_bytes} % 32u) != 0u) {
         --tail_bytes;
         p_dest[idx{tail_bytes}] = p_src[idx{tail_bytes}];
      }

      while (tail_bytes >= step_size) {
         tail_bytes -= step_size;

#pragma unroll 8
         for (idx vector_index = 0u; vector_index < 8u; ++vector_index) {
            char const* _Nonnull const p_byte =
               p_src + tail_bytes + (vector_index * sizeof(Vector));
            vectors[vector_index].load_unaligned(p_byte);
         }

#pragma unroll 8
         for (idx vector_index = 0u; vector_index < 8u; ++vector_index) {
            vectors[vector_index].store_non_temporal(
               p_dest + tail_bytes + (vector_index * sizeof(Vector)));
         }
      }

      x64::sfence();
   }

   if (tail_bytes > 0u) {
      copy_memory_backward_medium(p_source, p_destination, idx{tail_bytes});
   }
}

}  // namespace cat::detail
