// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/bit>
#include <cat/debug>
#include <cat/simd>

#include "copy_memory_medium.tpp"

namespace cat::detail {

template <typename Vector>
void
copy_memory_large(
   byte const* _Nonnull p_source, byte* _Nonnull p_destination, idx bytes
) {
   iword bytes_remaining = bytes;

   cat::uintptr<char> p_source_handle{
      const_cast<char*>(reinterpret_cast<char const*>(p_source))
   };
   cat::uintptr<char> p_destination_handle{
      reinterpret_cast<char*>(p_destination)
   };

   constexpr idx l3_cache_size = 2_umi;
   constexpr idx step_size = sizeof(Vector) * 8u;

   constexpr ualign simd_align = alignof(Vector);
   constexpr uword simd_align_bytes = simd_align;
   uword const dest_mod =
      (p_destination_handle & simd_align_bytes) % simd_align_bytes;
   uword const src_mod =
      (p_source_handle & simd_align_bytes) % simd_align_bytes;

   uword const padding =
      src_mod == dest_mod
         ? (simd_align_bytes - dest_mod) % simd_align_bytes
         : align_up(p_destination_handle, simd_align) - p_destination_handle;

   cat::copy_memory_scalar(p_source, p_destination, padding.to_idx().assert());

   p_source_handle += padding;
   p_destination_handle += padding;
   bytes_remaining -= padding;

   // The destination is now SIMD-aligned. Source alignment is irrelevant since
   // every load below is unaligned, so no scalar source peel is performed. This
   // keeps misaligned-source and overlapping memmove copies off the byte path.

   if (bytes_remaining <= step_size) {
      copy_memory_medium(
         reinterpret_cast<byte const* _Nonnull>(p_source_handle.get()),
         reinterpret_cast<byte* _Nonnull>(p_destination_handle.get()),
         bytes_remaining.to_idx().assert()
      );
      return;
   }

   bool const dest_simd_aligned =
      cat::is_aligned(p_destination_handle.get(), simd_align);

   Vector vectors[8];

   if (bytes_remaining <= l3_cache_size) {
      prefetch_mid(p_source_handle.get() + step_size);
      prefetch_mid(p_source_handle.get() + (step_size * 2u));

      while (bytes_remaining >= step_size) {
#pragma unroll 8
         for (idx vector_index = 0u; vector_index < 8u; ++vector_index) {
            char const* _Nonnull const p_byte =
               p_source_handle.get() + (vector_index * sizeof(Vector));
            vectors[vector_index].load_unaligned(p_byte);
         }

         prefetch_mid(p_source_handle.get() + (step_size * 2u));
         prefetch_mid(p_source_handle.get() + (step_size * 3u));

         if (dest_simd_aligned) {
            Vector* _Nonnull const p_dest =
               __builtin_bit_cast(Vector* _Nonnull, p_destination_handle.get());

#pragma unroll 8
            for (idx vector_index = 0u; vector_index < 8u; ++vector_index) {
               p_dest[vector_index] = vectors[vector_index];
            }
         } else {
#pragma unroll 8
            for (idx vector_index = 0u; vector_index < 8u; ++vector_index) {
               vectors[vector_index].store_unaligned(
                  p_destination_handle.get() + (vector_index * sizeof(Vector))
               );
            }
         }

         p_source_handle += step_size;
         p_destination_handle += step_size;
         bytes_remaining -= step_size;
      }
   } else if (dest_simd_aligned) {
      prefetch_far(p_source_handle.get() + (step_size * 2u));
      prefetch_far(p_source_handle.get() + (step_size * 4u));
      prefetch_far(p_source_handle.get() + (step_size * 6u));

      while (bytes_remaining >= step_size) {
#pragma unroll 8
         for (idx vector_index = 0u; vector_index < 8u; ++vector_index) {
            char const* _Nonnull const p_byte =
               p_source_handle.get() + (vector_index * sizeof(Vector));
            vectors[vector_index].load_unaligned(p_byte);
         }

         prefetch_far(p_source_handle.get() + (step_size * 4u));
         prefetch_far(p_source_handle.get() + (step_size * 6u));
         prefetch_far(p_source_handle.get() + (step_size * 8u));
         p_source_handle += step_size;

#pragma unroll 8
         for (idx vector_index = 0u; vector_index < 8u; ++vector_index) {
            vectors[vector_index].store_non_temporal(
               p_destination_handle.get() + (vector_index * sizeof(Vector))
            );
         }

         p_destination_handle += step_size;
         bytes_remaining -= step_size;
      }

      x64::sfence();
   } else {
      prefetch_far(p_source_handle.get() + (step_size * 2u));
      prefetch_far(p_source_handle.get() + (step_size * 4u));

      while (bytes_remaining >= step_size) {
#pragma unroll 8
         for (idx vector_index = 0u; vector_index < 8u; ++vector_index) {
            char const* _Nonnull const p_byte =
               p_source_handle.get() + (vector_index * sizeof(Vector));
            vectors[vector_index].load_unaligned(p_byte);
         }

         prefetch_far(p_source_handle.get() + (step_size * 4u));
         prefetch_far(p_source_handle.get() + (step_size * 6u));

#pragma unroll 8
         for (idx vector_index = 0u; vector_index < 8u; ++vector_index) {
            vectors[vector_index].store_unaligned(
               p_destination_handle.get() + (vector_index * sizeof(Vector))
            );
         }

         p_source_handle += step_size;
         p_destination_handle += step_size;
         bytes_remaining -= step_size;
      }
   }

   if (bytes_remaining > 0) {
      copy_memory_medium(
         reinterpret_cast<byte const* _Nonnull>(p_source_handle.get()),
         reinterpret_cast<byte* _Nonnull>(p_destination_handle.get()),
         bytes_remaining.to_idx().assert()
      );
   }
}

}  // namespace cat::detail
