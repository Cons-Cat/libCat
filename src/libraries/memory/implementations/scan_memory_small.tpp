// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/bit>
#include <cat/simd>

#include "scan_memory_chunk.tpp"

namespace cat::detail {

[[nodiscard]]
inline auto
scan_memory_chunk(
   byte const* _Nonnull p_left, byte const* _Nonnull p_right, idx bytes
) -> maybe<idx> {
   for (idx byte_index = 0u; byte_index < bytes; ++byte_index) {
      if (p_left[byte_index].value != p_right[byte_index].value) {
         return byte_index;
      }
   }

   return nullopt;
}

template <idx chunk_bytes>
[[nodiscard, gnu::always_inline]]
inline auto
scan_memory_load(byte const* _Nonnull p_data) -> uint_fixed<chunk_bytes> {
   uint_fixed<chunk_bytes> word;
   __builtin_memcpy_inline(&word, p_data, chunk_bytes);
   return word;
}

[[nodiscard]]
inline auto
scan_memory_word(byte const* _Nonnull p_left, byte const* _Nonnull p_right)
   -> maybe<idx> {
   uword const differing_bits =
      scan_memory_load<8u>(p_left) ^ scan_memory_load<8u>(p_right);
   if (differing_bits == 0u) {
      return nullopt;
   }

   // Find the first differing bit.
   return is_target_little_endian ? countr_zero(differing_bits) / char_bits
                                  : countl_zero(differing_bits) / char_bits;
}

// Power-of-2 branch tree for sizes `<= 32`.
template <memory_scan scan>
[[nodiscard, clang::no_builtin("memcmp")]]
inline auto
scan_memory_small(
   byte const* _Nonnull p_left, byte const* _Nonnull p_right, idx byte_count
) -> memory_scan_result<scan> {
   if (byte_count == 0u) {
      return memory_scan_none<scan>();
   }

   if (byte_count == 1u) {
      bool const differs = p_left[0].value != p_right[0].value;
      if constexpr (scan == memory_scan::mismatch) {
         if (differs) {
            return 0u;
         }

         return nullopt;
      } else {
         return differs;
      }
   }

   // Compare an overlapping head and tail.
   auto compare_head_tail = [&]<idx chunk_bytes> -> memory_scan_result<scan> {
      idx const tail = idx(byte_count - chunk_bytes);
      auto const head_difference = scan_memory_load<chunk_bytes>(p_left)
                                   ^ scan_memory_load<chunk_bytes>(p_right);
      auto const tail_difference =
         scan_memory_load<chunk_bytes>(p_left + tail)
         ^ scan_memory_load<chunk_bytes>(p_right + tail);

      if constexpr (scan == memory_scan::equality) {
         // Bit-or the differences to branch once.
         return (head_difference | tail_difference) != 0u;
      } else {
         if (head_difference != 0u) {
            return scan_memory_chunk(p_left, p_right, chunk_bytes);
         }

         if (tail_difference != 0u) {
            return scan_memory_at<scan>(
               scan_memory_chunk(p_left + tail, p_right + tail, chunk_bytes),
               tail
            );
         }

         return nullopt;
      }
   };

   if (byte_count < 4u) {
      return compare_head_tail.template operator()<2u>();
   }

   if (byte_count < 8u) {
      return compare_head_tail.template operator()<4u>();
   }

   if (byte_count < 16u) {
      return compare_head_tail.template operator()<8u>();
   }

   // Four words cover `16..32` bytes.
   array const offsets = {
      0_idx, 8_idx, idx(byte_count - 16u), idx(byte_count - 8u)
   };
   if constexpr (scan == memory_scan::equality) {
      uword difference = 0u;
      for (idx const offset : offsets) {
         difference |= scan_memory_load<8u>(p_left + offset)
                       ^ scan_memory_load<8u>(p_right + offset);
      }

      return difference != 0u;
   } else {
      for (idx const offset : offsets) {
         if (
            maybe<idx> const result =
               scan_memory_word(p_left + offset, p_right + offset);
            result.has_value()
         ) {
            return result.value() + offset;
         }
      }

      return nullopt;
   }
}

}  // namespace cat::detail
