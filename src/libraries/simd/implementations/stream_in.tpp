#pragma once

#include <cat/simd>

// TODO: Constrain parameter with a vector concept.
// TODO: This code can be simplified a lot.
// TODO: Clang has non-temporal intrinsics.
// Non-temporally copy a vector into some address.
template <typename T>
void
cat::stream_in(void* p_destination, T const* source) {
   // TODO: Make an integral-vector concept to simplify this.
   // Streaming 4-byte floats.
   if constexpr (cat::is_same<T, cat::float4x4>) {
      __builtin_ia32_movntps(p_destination, source);
   } else if constexpr (cat::is_same<T, cat::float4x8>) {
      __builtin_ia32_movntps256(p_destination, source);
   }
   // Streaming 8-byte floats.
   else if constexpr (cat::is_same<T, cat::float8x2>) {
      __builtin_ia32_movntpd(p_destination, source);
   } else if constexpr (cat::is_same<T, cat::float8x4>) {
      __builtin_ia32_movntpd256(p_destination, source);
   }
   // Streaming 1-byte ints.
   else if constexpr (cat::is_same<T, cat::uint1x4>
                      || cat::is_same<T, cat::int1x4>) {
      __builtin_ia32_movnti(p_destination, source);
   } else if constexpr (cat::is_same<T, cat::uint1x8>
                        || cat::is_same<T, cat::int1x8>) {
      __builtin_ia32_movntq(p_destination, source);
   } else if constexpr (cat::is_same<T, cat::uint1x16>
                        || cat::is_same<T, cat::int1x16>) {
      __builtin_ia32_movntq128(p_destination, source);
   } else if constexpr (cat::is_same<T, cat::uint1x32>
                        || cat::is_same<T, cat::int1x32>) {
      __builtin_ia32_movntq256(p_destination, source);
   }
   // Streaming 2-byte ints.
   else if constexpr (cat::is_same<T, cat::uint2x2>
                      || cat::is_same<T, cat::int2x2>) {
      __builtin_ia32_movnti(p_destination, source);
   } else if constexpr (cat::is_same<T, cat::uint2x4>
                        || cat::is_same<T, cat::int2x4>) {
      __builtin_ia32_movntq(p_destination, source);
   } else if constexpr (cat::is_same<T, cat::uint2x8>
                        || cat::is_same<T, cat::int2x8>) {
      __builtin_ia32_movntq128(p_destination, source);
   } else if constexpr (cat::is_same<T, cat::uint2x16>
                        || cat::is_same<T, cat::int2x16>) {
      __builtin_ia32_movntq256(p_destination, source);
   }
   // Streaming 4-byte ints.
   else if constexpr (cat::is_same<T, cat::uint4x2>
                      || cat::is_same<T, cat::int4x2>) {
      __builtin_ia32_movntq(p_destination, source);
   } else if constexpr (cat::is_same<T, cat::uint4x4>
                        || cat::is_same<T, cat::int4x4>) {
      __builtin_ia32_movntq128(p_destination, source);
   } else if constexpr (cat::is_same<T, cat::uint4x8>
                        || cat::is_same<T, cat::int4x8>) {
      __builtin_ia32_movntdq256(p_destination, source);
   }
   // Streaming 8-byte ints.
   else if constexpr (cat::is_same<T, cat::uint1x2>
                      || cat::is_same<T, cat::int1x2>) {
      __builtin_ia32_movntq128(p_destination, source);
   } else if constexpr (cat::is_same<T, cat::uint1x4>
                        || cat::is_same<T, cat::int1x4>) {
      __builtin_ia32_movntdq256(p_destination, source);
   }
}
