#pragma once

#include <cat/simd>

// SSSE3 intrinsics not exposed as portable `cat::simd_*` functors. The
// pairwise horizontal integer add/sub (`phaddw`/`phaddd`/`phsubw`/`phsubd`)
// and their saturating 16-bit variants (`phaddsw`/`phsubsw`) are exposed as
// the portable `cat::simd_pairwise_add`/`simd_pairwise_sub`/
// `simd_pairwise_add_saturating`/`simd_pairwise_sub_saturating` functors
// (interleaved semantics), distinct from the raw x86
// `ssse3_horizontal_*` defined below. `pabs*` is already covered by
// `cat::simd_abs`, so it is not re-exposed.

namespace x64 {

// This corresponds to `pshufb`/`_mm_shuffle_epi8()`.
// Per-byte shuffle of `input` according to each lane's index in `mask`.
template <typename T, is_sse_abi<T> Abi>
   requires(cat::is_integral<T> && !cat::is_bool<T> && sizeof(T) == 1)
[[gnu::target("ssse3")]]
constexpr auto
ssse3_shuffle_bytes(cat::simd<T, Abi> input, cat::simd<T, Abi> mask)
   -> cat::simd<T, Abi> {
   return cat::simd<T, Abi>(__builtin_ia32_pshufb128(input.raw, mask.raw));
}

// This corresponds to `pmaddubsw`/`_mm_maddubs_epi16()`.
// Multiply unsigned bytes by signed bytes and sum adjacent pairs into 2-byte
// lanes.
template <typename T1, is_sse_abi<T1> A1, typename T2, is_sse_abi<T2> A2>
   requires(
      cat::is_integral<T1> && !cat::is_bool<T1> && sizeof(T1) == 1
      && cat::is_integral<T2> && !cat::is_bool<T2> && sizeof(T2) == 1
   )
[[gnu::target("ssse3")]]
constexpr auto
ssse3_multiply_add_bytes(cat::simd<T1, A1> left, cat::simd<T2, A2> right)
   -> sse_simd<cat::int2> {
   return __builtin_ia32_pmaddubsw128(left.raw, right.raw);
}

// This corresponds to `pmulhrsw`/`_mm_mulhrs_epi16()`.
// Q15 fixed-point multiply with round and scale on 2-byte lanes.
template <typename T, is_sse_abi<T> Abi>
   requires(cat::is_integral<T> && !cat::is_bool<T> && sizeof(T) == 2)
[[gnu::target("ssse3")]]
constexpr auto
ssse3_multiply_round_scale(cat::simd<T, Abi> left, cat::simd<T, Abi> right)
   -> cat::simd<T, Abi> {
   return cat::simd<T, Abi>(__builtin_ia32_pmulhrsw128(left.raw, right.raw));
}

// This corresponds to
// `psignb`/`psignw`/`psignd`/`_mm_sign_epi8()`/`_mm_sign_epi16()`/`_mm_sign_epi32()`.
// Apply the sign of `sign` to each lane of `input`.
template <typename T, is_sse_abi<T> Abi>
   requires(cat::is_integral<T> && !cat::is_bool<T> && sizeof(T) <= 4)
[[gnu::target("ssse3")]]
constexpr auto
ssse3_sign(cat::simd<T, Abi> input, cat::simd<T, Abi> sign)
   -> cat::simd<T, Abi> {
   if constexpr (sizeof(T) == 1) {
      return cat::simd<T, Abi>(__builtin_ia32_psignb128(input.raw, sign.raw));
   } else if constexpr (sizeof(T) == 2) {
      return cat::simd<T, Abi>(__builtin_ia32_psignw128(input.raw, sign.raw));
   } else {
      return cat::simd<T, Abi>(__builtin_ia32_psignd128(input.raw, sign.raw));
   }
}

// This corresponds to `palignr`/`_mm_alignr_epi8()`.
// Concatenate [high : low] and right-shift by `byte_count` bytes, keeping the
// low 16 bytes.
template <cat::idx byte_count, typename T, is_sse_abi<T> Abi>
   requires(byte_count < 32u)
[[gnu::target("ssse3")]]
constexpr auto
ssse3_align_right(cat::simd<T, Abi> high, cat::simd<T, Abi> low)
   -> cat::simd<T, Abi> {
   return cat::simd<T, Abi>(__builtin_ia32_palignr128(
      // TODO: `.raw` shouldn't be needed here to cast to `unsigned`.
      high.raw, low.raw, static_cast<unsigned>(byte_count.raw)
   ));
}

// This corresponds to `phaddw`/`phaddd`/`_mm_hadd_epi16()`/`_mm_hadd_epi32()`.
// Horizontally add adjacent pairs of integer lanes across two vectors.
template <typename T, is_sse_abi<T> Abi>
   requires(
      cat::is_integral<T> && !cat::is_bool<T>
      && (sizeof(T) == 2 || sizeof(T) == 4)
   )
[[gnu::target("ssse3")]]
constexpr auto
ssse3_horizontal_add(cat::simd<T, Abi> left, cat::simd<T, Abi> right)
   -> cat::simd<T, Abi> {
   if constexpr (sizeof(T) == 2) {
      return cat::simd<T, Abi>(__builtin_ia32_phaddw128(left.raw, right.raw));
   } else {
      return cat::simd<T, Abi>(__builtin_ia32_phaddd128(left.raw, right.raw));
   }
}

// This corresponds to `phsubw`/`phsubd`/`_mm_hsub_epi16()`/`_mm_hsub_epi32()`.
// Horizontally subtract adjacent pairs of integer lanes across two vectors.
template <typename T, is_sse_abi<T> Abi>
   requires(
      cat::is_integral<T> && !cat::is_bool<T>
      && (sizeof(T) == 2 || sizeof(T) == 4)
   )
[[gnu::target("ssse3")]]
constexpr auto
ssse3_horizontal_sub(cat::simd<T, Abi> left, cat::simd<T, Abi> right)
   -> cat::simd<T, Abi> {
   if constexpr (sizeof(T) == 2) {
      return cat::simd<T, Abi>(__builtin_ia32_phsubw128(left.raw, right.raw));
   } else {
      return cat::simd<T, Abi>(__builtin_ia32_phsubd128(left.raw, right.raw));
   }
}

// This corresponds to `phaddsw`/`_mm_hadds_epi16()`.
// Signed-saturating horizontal add of 2-byte lanes.
template <typename T, is_sse_abi<T> Abi>
   requires(cat::is_integral<T> && !cat::is_bool<T> && sizeof(T) == 2)
[[gnu::target("ssse3")]]
constexpr auto
ssse3_horizontal_add_saturating(cat::simd<T, Abi> left, cat::simd<T, Abi> right)
   -> cat::simd<T, Abi> {
   return cat::simd<T, Abi>(__builtin_ia32_phaddsw128(left.raw, right.raw));
}

// This corresponds to `phsubsw`/`_mm_hsubs_epi16()`.
// Signed-saturating horizontal subtract of 2-byte lanes.
template <typename T, is_sse_abi<T> Abi>
   requires(cat::is_integral<T> && !cat::is_bool<T> && sizeof(T) == 2)
[[gnu::target("ssse3")]]
constexpr auto
ssse3_horizontal_sub_saturating(cat::simd<T, Abi> left, cat::simd<T, Abi> right)
   -> cat::simd<T, Abi> {
   return cat::simd<T, Abi>(__builtin_ia32_phsubsw128(left.raw, right.raw));
}

}  // namespace x64
