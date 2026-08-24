#pragma once

// SSE3 intrinsics not exposed as portable `cat::simd_*` functors. The
// pairwise horizontal add/sub (`hadd`/`hsub`) are exposed as the portable
// `cat::simd_pairwise_add`/`simd_pairwise_sub` functors (interleaved
// semantics), distinct from the raw x86 `sse3_horizontal_add`/
// `sse3_horizontal_sub` defined below. The lane-duplication intrinsics
// `movshdup`/`movsldup`/`movddup` are already covered by
// `cat::simd_duplicate_odd` and `cat::simd_duplicate_even`, so they are not
// re-exposed here.

#include <cat/simd>

namespace x64 {

// This corresponds to
// `addsubps`/`addsubpd`/`_mm_addsub_ps()`/`_mm_addsub_pd()`. Subtract
// odd-indexed lanes from even-indexed lanes, then add odd-indexed lanes.
template <typename T, is_sse_abi<T> Abi>
   requires(cat::is_floating_point<T>)
[[gnu::target("sse3")]]
constexpr auto
sse3_add_sub_alternating(cat::simd<T, Abi> left, cat::simd<T, Abi> right)
   -> cat::simd<T, Abi> {
   if constexpr (sizeof(T) == 4) {
      return cat::simd<T, Abi>(__builtin_ia32_addsubps(left.raw, right.raw));
   } else {
      return cat::simd<T, Abi>(__builtin_ia32_addsubpd(left.raw, right.raw));
   }
}

// This corresponds to `lddqu`/`_mm_lddqu_si128()`.
// Load 16 bytes from an unaligned address with a non-temporal cache hint.
template <typename T, is_sse_abi<T> Abi = sse_abi<T>>
[[gnu::target("sse3")]]
auto
sse3_load_unaligned_non_temporal(void const* _Nonnull p_source)
   -> cat::simd<T, Abi> {
   using raw_t = cat::simd<T, Abi>::raw_type;
   auto const bytes =
      __builtin_ia32_lddqu(static_cast<char const* _Nonnull>(p_source));
   return cat::simd<T, Abi>(__builtin_bit_cast(raw_t, bytes));
}

// This corresponds to `haddps`/`haddpd`/`_mm_hadd_ps()`/`_mm_hadd_pd()`.
// Horizontally add adjacent pairs of lanes across two vectors.
template <typename T, is_sse_abi<T> Abi>
   requires(cat::is_floating_point<T>)
[[gnu::target("sse3")]]
constexpr auto
sse3_horizontal_add(cat::simd<T, Abi> left, cat::simd<T, Abi> right)
   -> cat::simd<T, Abi> {
   if constexpr (sizeof(T) == 4) {
      return cat::simd<T, Abi>(__builtin_ia32_haddps(left.raw, right.raw));
   } else {
      return cat::simd<T, Abi>(__builtin_ia32_haddpd(left.raw, right.raw));
   }
}

// This corresponds to `hsubps`/`hsubpd`/`_mm_hsub_ps()`/`_mm_hsub_pd()`.
// Horizontally subtract adjacent pairs of lanes across two vectors.
template <typename T, is_sse_abi<T> Abi>
   requires(cat::is_floating_point<T>)
[[gnu::target("sse3")]]
constexpr auto
sse3_horizontal_sub(cat::simd<T, Abi> left, cat::simd<T, Abi> right)
   -> cat::simd<T, Abi> {
   if constexpr (sizeof(T) == 4) {
      return cat::simd<T, Abi>(__builtin_ia32_hsubps(left.raw, right.raw));
   } else {
      return cat::simd<T, Abi>(__builtin_ia32_hsubpd(left.raw, right.raw));
   }
}

}  // namespace x64
