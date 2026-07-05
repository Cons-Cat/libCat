// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/simd>

namespace cat::detail {

// Cosmopolitan-style small `memset` for sizes `<= 127`.
[[clang::no_builtin("memset")]]
inline void
fill_memory_small(byte* _Nonnull p_destination, byte byte_value, idx bytes) {
   char* p_dest = reinterpret_cast<char*>(p_destination);
   idx const byte_count = bytes;
   uint1 const fill_byte = byte_value.value;

   // Sizes up to 16 are filled with at most two overlapping fixed-width
   // stores. Each size class broadcasts the fill byte across a machine word by
   // multiplying it by a pattern of `0x01` bytes: `0x01010101... * byte` copies
   // `byte` into every byte lane, since each `0x01` contributes one shifted
   // copy.
   //
   // That word is then written to both the front and the back of the region.
   // When the size is not an exact multiple of the store width the two stores
   // overlap.
   //
   //   8..16 bytes: two 8-byte stores at [0] and [count - 8].
   //   4..7  bytes: two 4-byte stores at [0] and [count - 4].
   //   1..3  bytes: too small to overlap a fixed store safely, so fall back to
   //                a short byte loop.
   //
   // TODO: Use a `cat::swar` for byte broadcast when we have one.
   if (byte_count <= 16u) {
      if (byte_count >= 8u) {
         uword const fill_pattern = 0x01010101'01010101ull * fill_byte;
         __builtin_memcpy_inline(p_dest, &fill_pattern, 8);
         __builtin_memcpy_inline(p_dest + byte_count - 8, &fill_pattern, 8);
      } else if (byte_count >= 4u) {
         uint4 const fill_pattern = 0x01010101u * fill_byte;
         __builtin_memcpy_inline(p_dest, &fill_pattern, 4);
         __builtin_memcpy_inline(p_dest + byte_count - 4, &fill_pattern, 4);
      } else if (byte_count > 0u) {
         for (idx byte_index = 0u; byte_index < byte_count; ++byte_index) {
            p_destination[byte_index] = byte_value;
         }
      }
      return;
   }

   // 17..32 bytes: the same overlapping-store idea with two 16-byte SIMD
   // fills at the front and back.
   if (byte_count <= 32u) {
      char1x16 chunk;
      chunk.fill(byte_value);
      chunk.store_unaligned(p_dest);
      chunk.store_unaligned(p_dest + byte_count - 16);
      return;
   }

   char1x16 chunk;
   chunk.fill(byte_value);

   if (byte_count < 64u) {
      chunk.store_unaligned(p_dest);
      chunk.store_unaligned(p_dest + 16);
      chunk.store_unaligned(p_dest + byte_count - 32);
      chunk.store_unaligned(p_dest + byte_count - 16);
      return;
   }

   chunk.store_unaligned(p_dest);
   chunk.store_unaligned(p_dest + 16);
   chunk.store_unaligned(p_dest + 32);
   chunk.store_unaligned(p_dest + 48);
   chunk.store_unaligned(p_dest + byte_count - 64);
   chunk.store_unaligned(p_dest + byte_count - 48);
   chunk.store_unaligned(p_dest + byte_count - 32);
   chunk.store_unaligned(p_dest + byte_count - 16);
}

}  // namespace cat::detail
