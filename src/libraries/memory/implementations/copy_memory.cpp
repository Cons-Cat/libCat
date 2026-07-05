#include <cat/detail/movsb.hpp>

#include <cat/arithmetic>
#include <cat/cpuid>
#include <cat/memory>
#include <cat/simd>
#include <cat/simd_dispatch>

#include "copy_memory_large.tpp"
#include "copy_memory_medium.tpp"
#include "copy_memory_small.tpp"

namespace cat::detail {

void
copy_memory_impl(byte const* _Nonnull __restrict p_source,
                 byte* _Nonnull __restrict p_destination, idx bytes) {
   __builtin_assume_separate_storage(p_source, p_destination);

   if (bytes <= 127u) {
      copy_memory_small(p_source, p_destination, bytes);
      return;
   }

   // TODO: Explore the best path for 16-byte SIMD platforms.
   if (bytes < 1'024u) {
      copy_memory_medium(p_source, p_destination, bytes);
      return;
   }

   if (memory_rep_string_support.has_fsrm
       || memory_rep_string_support.has_erms) {
      x64::movsb(p_destination, p_source, bytes);
      return;
   }

   $simd_switch(
      $abi(avx2,
           {
              copy_memory_large<char1x_>(p_source, p_destination, bytes);
              x64::zero_upper_avx_registers();
           }),
      $abi(sse2,
           { copy_memory_large<char1x_>(p_source, p_destination, bytes); }));
}

}  // namespace cat::detail
