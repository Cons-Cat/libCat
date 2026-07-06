#include <cat/detail/movsb.hpp>

#include <cat/arithmetic>
#include <cat/bit>
#include <cat/cpuid>
#include <cat/memory>
#include <cat/simd>
#include <cat/simd_dispatch>

#include "fill_memory_medium.tpp"
#include "fill_memory_small.tpp"

namespace cat::detail {

template <typename Vector>
void
fill_memory_large(byte* _Nonnull p_destination, byte byte_value, idx bytes) {
   iword bytes_remaining = bytes;
   // TODO: `cat::simd` should support `cat::byte` better.

   cat::uintptr<char> p_handle{reinterpret_cast<char*>(p_destination)};
   constexpr idx step_size = sizeof(Vector) * 8u;
   constexpr ualign simd_align = alignof(Vector);
   constexpr uword simd_align_bytes = simd_align;

   if (bytes_remaining < step_size) {
      fill_memory_medium(p_destination, byte_value, bytes);
      return;
   }

   uword const dest_mod = (p_handle & simd_align_bytes) % simd_align_bytes;
   uword const padding = (simd_align_bytes - dest_mod) % simd_align_bytes;

   for (idx byte_index = 0u; byte_index < padding.to_idx().assert();
        ++byte_index) {
      p_destination[byte_index] = byte_value;
   }
   bytes_remaining -= padding;

   if (bytes_remaining <= 0) {
      return;
   }

   Vector splat;
   splat.fill(byte_value);

   constexpr idx l3_cache_size = 2_umi;
   bool const dest_simd_aligned = (p_handle % simd_align_bytes) == 0u;

   if (bytes_remaining <= l3_cache_size) {
      while (bytes_remaining >= step_size) {
         if (dest_simd_aligned) {
            Vector* _Nonnull const p_dest =
               __builtin_bit_cast(Vector* _Nonnull, p_handle.get());
#pragma unroll 8
            for (idx vector_index = 0u; vector_index < 8u; ++vector_index) {
               p_dest[vector_index] = splat;
            }
         } else {
#pragma unroll 8
            for (idx vector_index = 0u; vector_index < 8u; ++vector_index) {
               splat.store_unaligned(
                  p_handle.get() + (vector_index * sizeof(Vector))
               );
            }
         }

         p_handle += step_size;
         bytes_remaining -= step_size;
      }
   } else if (dest_simd_aligned) {
      while (bytes_remaining >= step_size) {
#pragma unroll 8
         for (idx vector_index = 0u; vector_index < 8u; ++vector_index) {
            splat.store_non_temporal(
               p_handle.get() + (vector_index * sizeof(Vector))
            );
         }

         p_handle += step_size;
         bytes_remaining -= step_size;
      }
      x64::sfence();
   } else {
      while (bytes_remaining >= step_size) {
#pragma unroll 8
         for (idx vector_index = 0u; vector_index < 8u; ++vector_index) {
            splat.store_unaligned(
               p_handle.get() + (vector_index * sizeof(Vector))
            );
         }

         p_handle += step_size;
         bytes_remaining -= step_size;
      }
   }

   if (bytes_remaining > 0) {
      fill_memory_medium(
         reinterpret_cast<byte* _Nonnull>(p_handle.get()), byte_value,
         bytes_remaining.to_idx().assert()
      );
   }
}

void
fill_memory_impl(byte* _Nonnull p_destination, byte byte_value, idx bytes) {
   if (bytes <= 127u) {
      fill_memory_small(p_destination, byte_value, bytes);
      return;
   }

   // TODO: Explore the best path for 16-byte SIMD platforms.
   if (bytes < 2'048u) {
      fill_memory_medium(p_destination, byte_value, bytes);
      return;
   }

   if (
      memory_rep_string_support.has_fsrm || memory_rep_string_support.has_erms
   ) {
      x64::stosb(p_destination, byte_value, bytes);
      return;
   }

   $simd_switch(
      $abi(
         avx2,
         {
            fill_memory_large<char1x_>(p_destination, byte_value, bytes);
            x64::zero_upper_avx_registers();
         }
      ),
      $abi(sse2, {
         fill_memory_large<char1x_>(p_destination, byte_value, bytes);
      })
   );
}

}  // namespace cat::detail
