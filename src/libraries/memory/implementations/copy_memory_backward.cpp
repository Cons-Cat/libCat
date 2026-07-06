#include <cat/memory>
#include <cat/simd>
#include <cat/simd_dispatch>

#include "copy_memory_backward_large.tpp"
#include "copy_memory_large.tpp"
#include "copy_memory_medium.tpp"
#include "copy_memory_small.tpp"

namespace cat::detail {

void
copy_memory_forward_overlap_impl(
   byte const* _Nonnull p_source, byte* _Nonnull p_destination, idx bytes
) {
   if (bytes <= 127u) {
      copy_memory_small(p_source, p_destination, bytes);
      return;
   }

   // TODO: Explore the best path for 16-byte SIMD platforms.
   if (bytes < 512u) {
      copy_memory_medium(p_source, p_destination, bytes);
      return;
   }

   $simd_switch(
      $abi(
         avx2,
         {
            copy_memory_large<char1x_>(p_source, p_destination, bytes);
            x64::zero_upper_avx_registers();
         }
      ),
      $abi(sse2, {
         copy_memory_large<char1x_>(p_source, p_destination, bytes);
      })
   );
}

void
copy_memory_backward_impl(
   byte const* _Nonnull p_source, byte* _Nonnull p_destination, idx bytes
) {
   if (p_destination <= p_source) {
      if (p_source + bytes <= p_destination) {
         copy_memory_impl(p_source, p_destination, bytes);
         return;
      }
      copy_memory_forward_overlap_impl(p_source, p_destination, bytes);
      return;
   }

   if (bytes <= 127u) {
      copy_memory_small(p_source, p_destination, bytes);
      return;
   }

   // TODO: Explore the best path for 16-byte SIMD platforms.
   if (bytes < 512u) {
      copy_memory_backward_medium(p_source, p_destination, bytes);
      return;
   }

   $simd_switch(
      $abi(
         avx2,
         {
            copy_memory_backward_large<char1x_>(p_source, p_destination, bytes);
            x64::zero_upper_avx_registers();
         }
      ),
      $abi(sse2, {
         copy_memory_backward_large<char1x_>(p_source, p_destination, bytes);
      })
   );
}

}  // namespace cat::detail
