#include <cat/arithmetic>
#include <cat/array>
#include <cat/bit>
#include <cat/debug>
#include <cat/memory>
#include <cat/simd>

namespace cat::detail {

// Vectorized implementation of `cat::copy_memory` for runtime.
void
copy_memory_impl(void const* p_source, void* p_destination, idx bytes) {
   iword bytes_remaining = bytes;
   using simd_vector = uint8x_;

   cat::uintptr<unsigned char> p_source_handle{
      const_cast<unsigned char*>(static_cast<unsigned char const*>(p_source))};
   cat::uintptr<unsigned char> p_destination_handle{
      static_cast<unsigned char*>(p_destination)};
   constexpr idx l3_cache_size = 2_umi;
   constexpr idx step_size = sizeof(simd_vector) * 8u;
   uword const simd_align = alignof(simd_vector);
   uword const dest_mod = (p_destination_handle & simd_align) % simd_align;
   uword const src_mod = (p_source_handle & simd_align) % simd_align;

   // If the copy is too short for vector stores, do it scalar.
   if (bytes_remaining <= step_size) {
      cat::copy_memory_scalar(p_source, p_destination,
                              bytes_remaining.to_idx().assert());
      return;
   }

   // When source and destination share the same residue mod `simd_align`,
   // padding the destination also aligns the source. Otherwise peel the
   // destination first, then peel additional `bytes_remaining` until the source
   // is aligned for `load_aligned` (stores may then use `store_unaligned`)
   uword const padding =
      src_mod == dest_mod
         ? (simd_align - dest_mod) % simd_align
         : align_up(p_destination_handle, simd_align) - p_destination_handle;

   // We should be in range of `idx` here.
   cat::copy_memory_scalar(p_source, p_destination, padding.to_idx().assert());

   p_source_handle += padding;
   p_destination_handle += padding;
   bytes_remaining -= padding;

   cat::uintptr<unsigned char> const src_after_padding = p_source_handle;
   idx const src_mis_after_pad{src_after_padding % simd_align};
   idx const source_peel{
      src_mis_after_pad == 0u ? 0u : simd_align - src_mis_after_pad};

   if (source_peel != 0u) {
      if (bytes_remaining < source_peel) {
         cat::copy_memory_scalar(p_source_handle.get(),
                                 p_destination_handle.get(),
                                 bytes_remaining.to_idx().assert());
         return;
      }
      cat::copy_memory_scalar(p_source_handle.get(), p_destination_handle.get(),
                              source_peel);
      p_source_handle += source_peel;
      p_destination_handle += source_peel;
      bytes_remaining -= source_peel;
   }

   if (bytes_remaining <= step_size) {
      cat::copy_memory_scalar(p_source_handle.get(), p_destination_handle.get(),
                              bytes_remaining.to_idx().assert());
      return;
   }

   bool const dest_simd_aligned = (p_destination_handle % simd_align) == 0u;

   simd_vector vectors[8];

   // This routine is optimized for buffers in L3 cache. Streaming is
   // slower there.
   if (bytes_remaining <= l3_cache_size) {
      while (bytes_remaining >= step_size) {
         // Load 8 vectors, then increment the source pointer by that size.
#pragma unroll 8
         for (idx i = 0u; i < 8u; ++i) {
            unsigned char const* const p_byte =
               p_source_handle.get() + (i * sizeof(simd_vector));
            simd_vector::memory_lane const* const p_chunk =
               reinterpret_cast<simd_vector::memory_lane const*>(p_byte);
            vectors[i].load_aligned(p_chunk);
         }
         prefetch_for_one_read(p_source_handle.get() + (step_size * 2u));

         if (dest_simd_aligned) {
#pragma unroll 8
            for (idx i = 0u; i < 8u; ++i) {
               __builtin_bit_cast(simd_vector*, p_destination_handle.get())[i] =
                  vectors[i];
            }
         } else {
#pragma unroll 8
            for (idx i = 0u; i < 8u; ++i) {
               vectors[i].store_unaligned(
                  reinterpret_cast<simd_vector::memory_lane*>(
                     p_destination_handle.get() + (i * sizeof(simd_vector))));
            }
         }
         p_source_handle += step_size;
         p_destination_handle += step_size;
         bytes_remaining -= step_size;
      }
   }

   // This routine is run when the memory source cannot fit in cache.
   else if (dest_simd_aligned) {
      prefetch_for_one_read(p_source_handle.get() + (step_size * 2u));
      // TODO: This code block has fallen far out of date.
      // TODO: This could be improved by using aligned-streaming when
      // possible.
      while (bytes_remaining >= step_size) {
#pragma unroll 8
         for (idx i = 0u; i < 8u; ++i) {
            unsigned char const* const p_byte =
               p_source_handle.get() + (i * sizeof(simd_vector));
            simd_vector::memory_lane const* const p_chunk =
               reinterpret_cast<simd_vector::memory_lane const*>(p_byte);
            vectors[i].load_aligned(p_chunk);
         }
         prefetch_for_one_read(p_source_handle.get() + (step_size * 2u));
         p_source_handle += step_size;
#pragma unroll 8
         for (idx i = 0u; i < 8u; ++i) {
            stream_in(p_destination_handle.get() + (i * sizeof(simd_vector)),
                      &vectors[i]);
         }
         p_destination_handle += step_size;
         bytes_remaining -= step_size;
      }

      x64::sfence();
   } else {
      prefetch_for_one_read(p_source_handle.get() + (step_size * 2u));
      while (bytes_remaining >= step_size) {
#pragma unroll 8
         for (idx i = 0u; i < 8u; ++i) {
            unsigned char const* const p_byte =
               p_source_handle.get() + (i * sizeof(simd_vector));
            simd_vector::memory_lane const* const p_chunk =
               reinterpret_cast<simd_vector::memory_lane const*>(p_byte);
            vectors[i].load_aligned(p_chunk);
         }
         prefetch_for_one_read(p_source_handle.get() + (step_size * 2u));

#pragma unroll 8
         for (idx i = 0u; i < 8u; ++i) {
            vectors[i].store_unaligned(
               reinterpret_cast<simd_vector::memory_lane*>(
                  p_destination_handle.get() + (i * sizeof(simd_vector))));
         }
         p_source_handle += step_size;
         p_destination_handle += step_size;
         bytes_remaining -= step_size;
      }
   }

   cat::copy_memory_scalar(p_source_handle.get(), p_destination_handle.get(),
                           bytes_remaining.to_idx().assert());
   x64::zero_upper_avx_registers();
}

}  // namespace cat::detail
