#pragma once

// libCat provides convenient type aliases for portable SIMD types and ABIs.
// These are placed in `cat::arithmetic`, alongside the scalar integer
// and floating point type aliases.
// `$simd_switch` `$abi` cases introduce a namespace that supercedes these
// types' unqualified lookup, which allows otherwise portable-looking SIMD code
// to "just work" in platform-specific optimized code.
//
// This file provides `CAT_SIMD_ALIASES(ABI)` which declares the
// following full SIMD alias sets inside a surrounding namespace:
//
//   * `native_simd<T>` / `native_simd_mask<T>` and their `_unaligned`
//     variants pin to `ABI<T>`.
//
//   * `deduce_simd<T, lanes>` / `deduce_simd_mask<T, lanes>` and their
//     `_unaligned` variants pass `ABI<T>` to `simd_abi::deduce`
//     as an extra candidate, biasing the deducer toward the arch.
//
//   * `fixed_size_simd<T, lanes>` and `scalar_simd<T>` are independent
//     of `ABI` (they always use `simd_abi::fixed_size` / `simd_abi::scalar`).
//
//   * Per-element aliases (`int4x_`, `int1x16`, `float4x_`,
//     `int_unalign_2x4`, ...) are spelled in terms of those four
//     families, so they pick up the `ABI` substitution too.
#define CAT_SIMD_ALIASES(ABI)                                                 \
   /* Aligned families. */                                                    \
   template <typename T, ::cat::idx lanes>                                    \
   using fixed_size_simd =                                                    \
      ::cat::simd<T, ::cat::simd_abi::fixed_size<T, lanes>>;                  \
   template <typename T, ::cat::idx lanes>                                    \
   using fixed_size_simd_mask =                                               \
      ::cat::simd_mask<T, ::cat::simd_abi::fixed_size<T, lanes>>;             \
   template <typename T, ::cat::idx lanes>                                    \
   using deduce_simd =                                                        \
      ::cat::simd<T, ::cat::simd_abi::deduce<T, lanes, ABI<T>>>;              \
   template <typename T, ::cat::idx lanes>                                    \
   using deduce_simd_mask =                                                   \
      ::cat::simd_mask<T, ::cat::simd_abi::deduce<T, lanes, ABI<T>>>;         \
   template <typename T>                                                      \
   using native_simd = ::cat::simd<T, ABI<T>>;                                \
   template <typename T>                                                      \
   using native_simd_mask = ::cat::simd_mask<T, ABI<T>>;                      \
   template <typename T>                                                      \
   using scalar_simd = ::cat::simd<T, ::cat::simd_abi::scalar<T>>;            \
   template <typename T>                                                      \
   using scalar_simd_mask = ::cat::simd_mask<T, ::cat::simd_abi::scalar<T>>;  \
                                                                              \
   /* Unaligned families. */                                                  \
   template <typename T, ::cat::idx lanes>                                    \
   using fixed_size_unaligned_simd = ::cat::simd<                             \
      T, ::cat::simd_abi::unaligned<::cat::simd_abi::fixed_size<T, lanes>>>;  \
   template <typename T, ::cat::idx lanes>                                    \
   using fixed_size_unaligned_simd_mask = ::cat::simd_mask<                   \
      T, ::cat::simd_abi::unaligned<::cat::simd_abi::fixed_size<T, lanes>>>;  \
   template <typename T, ::cat::idx lanes>                                    \
   using deduce_unaligned_simd = ::cat::simd<                                 \
      T,                                                                      \
      ::cat::simd_abi::unaligned<::cat::simd_abi::deduce<T, lanes, ABI<T>>>>; \
   template <typename T, ::cat::idx lanes>                                    \
   using deduce_unaligned_simd_mask = ::cat::simd_mask<                       \
      T,                                                                      \
      ::cat::simd_abi::unaligned<::cat::simd_abi::deduce<T, lanes, ABI<T>>>>; \
   template <typename T>                                                      \
   using native_unaligned_simd =                                              \
      ::cat::simd<T, ::cat::simd_abi::unaligned<ABI<T>>>;                     \
   template <typename T>                                                      \
   using native_unaligned_simd_mask =                                         \
      ::cat::simd_mask<T, ::cat::simd_abi::unaligned<ABI<T>>>;                \
   template <typename T>                                                      \
   using compatible_unaligned_simd = ::cat::simd<                             \
      T, ::cat::simd_abi::unaligned<::cat::simd_abi::compatible<T>>>;         \
   template <typename T>                                                      \
   using compatible_unaligned_simd_mask = ::cat::simd_mask<                   \
      T, ::cat::simd_abi::unaligned<::cat::simd_abi::compatible<T>>>;         \
   template <typename T>                                                      \
   using scalar_unaligned_simd =                                              \
      ::cat::simd<T, ::cat::simd_abi::unaligned<::cat::simd_abi::scalar<T>>>; \
   template <typename T>                                                      \
   using scalar_unaligned_simd_mask = ::cat::simd_mask<                       \
      T, ::cat::simd_abi::unaligned<::cat::simd_abi::scalar<T>>>;             \
                                                                              \
   /* Fixed-size per-element aliases. `_xN` suffixes name an explicit lane */ \
   /* count. Their ABI is `simd_abi::deduce` over `ABI`, so the host's     */ \
   /* matching arch wins. Unaligned variants spelled                      */  \
   /* `_unalign_<size>x<lanes>`. The `_x_` placeholder-lane aliases live  */  \
   /* in the separate `CAT_SIMD_PLACEHOLDER_ALIASES` macro. Those vary    */  \
   /* directly with `ABI<T>` and only make sense inside an arch namespace,*/  \
   /* where `ABI` is per-arch.                                            */  \
                                                                              \
   /* int1 / uint1 vectors: */                                                \
   using int1x2 = deduce_simd<::cat::int1, 2u>;                               \
   using int1x4 = deduce_simd<::cat::int1, 4u>;                               \
   using int1x8 = deduce_simd<::cat::int1, 8u>;                               \
   using int1x16 = deduce_simd<::cat::int1, 16u>;                             \
   using int1x32 = deduce_simd<::cat::int1, 32u>;                             \
   using uint1x2 = deduce_simd<::cat::uint1, 2u>;                             \
   using uint1x4 = deduce_simd<::cat::uint1, 4u>;                             \
   using uint1x8 = deduce_simd<::cat::uint1, 8u>;                             \
   using uint1x16 = deduce_simd<::cat::uint1, 16u>;                           \
   using uint1x32 = deduce_simd<::cat::uint1, 32u>;                           \
   using int_unalign_1x2 = deduce_unaligned_simd<::cat::int1, 2u>;            \
   using int_unalign_1x4 = deduce_unaligned_simd<::cat::int1, 4u>;            \
   using int_unalign_1x8 = deduce_unaligned_simd<::cat::int1, 8u>;            \
   using int_unalign_1x16 = deduce_unaligned_simd<::cat::int1, 16u>;          \
   using int_unalign_1x32 = deduce_unaligned_simd<::cat::int1, 32u>;          \
   using uint_unalign_1x2 = deduce_unaligned_simd<::cat::uint1, 2u>;          \
   using uint_unalign_1x4 = deduce_unaligned_simd<::cat::uint1, 4u>;          \
   using uint_unalign_1x8 = deduce_unaligned_simd<::cat::uint1, 8u>;          \
   using uint_unalign_1x16 = deduce_unaligned_simd<::cat::uint1, 16u>;        \
   using uint_unalign_1x32 = deduce_unaligned_simd<::cat::uint1, 32u>;        \
                                                                              \
   /* char (separate type from int1; element of the `_string`-style API). */  \
   /* TODO: think over the string vectorization API; support `char2x_`. */    \
   using char1x16 = deduce_simd<char, 16u>;                                   \
   using char1x32 = deduce_simd<char, 32u>;                                   \
   using char_unalign_1x16 = deduce_unaligned_simd<char, 16u>;                \
   using char_unalign_1x32 = deduce_unaligned_simd<char, 32u>;                \
                                                                              \
   /* int2 / uint2 vectors: */                                                \
   using int2x2 = deduce_simd<::cat::int2, 2u>;                               \
   using int2x4 = deduce_simd<::cat::int2, 4u>;                               \
   using int2x8 = deduce_simd<::cat::int2, 8u>;                               \
   using int2x16 = deduce_simd<::cat::int2, 16u>;                             \
   using uint2x2 = deduce_simd<::cat::uint2, 2u>;                             \
   using uint2x4 = deduce_simd<::cat::uint2, 4u>;                             \
   using uint2x8 = deduce_simd<::cat::uint2, 8u>;                             \
   using uint2x16 = deduce_simd<::cat::uint2, 16u>;                           \
   using int_unalign_2x2 = deduce_unaligned_simd<::cat::int2, 2u>;            \
   using int_unalign_2x4 = deduce_unaligned_simd<::cat::int2, 4u>;            \
   using int_unalign_2x8 = deduce_unaligned_simd<::cat::int2, 8u>;            \
   using int_unalign_2x16 = deduce_unaligned_simd<::cat::int2, 16u>;          \
   using uint_unalign_2x2 = deduce_unaligned_simd<::cat::uint2, 2u>;          \
   using uint_unalign_2x4 = deduce_unaligned_simd<::cat::uint2, 4u>;          \
   using uint_unalign_2x8 = deduce_unaligned_simd<::cat::uint2, 8u>;          \
   using uint_unalign_2x16 = deduce_unaligned_simd<::cat::uint2, 16u>;        \
                                                                              \
   /* int4 / uint4 vectors: */                                                \
   using int4x2 = deduce_simd<::cat::int4, 2u>;                               \
   using int4x4 = deduce_simd<::cat::int4, 4u>;                               \
   using int4x8 = deduce_simd<::cat::int4, 8u>;                               \
   using uint4x2 = deduce_simd<::cat::uint4, 2u>;                             \
   using uint4x4 = deduce_simd<::cat::uint4, 4u>;                             \
   using uint4x8 = deduce_simd<::cat::uint4, 8u>;                             \
   using int_unalign_4x2 = deduce_unaligned_simd<::cat::int4, 2u>;            \
   using int_unalign_4x4 = deduce_unaligned_simd<::cat::int4, 4u>;            \
   using int_unalign_4x8 = deduce_unaligned_simd<::cat::int4, 8u>;            \
   using uint_unalign_4x2 = deduce_unaligned_simd<::cat::uint4, 2u>;          \
   using uint_unalign_4x4 = deduce_unaligned_simd<::cat::uint4, 4u>;          \
   using uint_unalign_4x8 = deduce_unaligned_simd<::cat::uint4, 8u>;          \
                                                                              \
   /* int8 / uint8 vectors: */                                                \
   using int8x2 = deduce_simd<::cat::int8, 2u>;                               \
   using int8x4 = deduce_simd<::cat::int8, 4u>;                               \
   using uint8x2 = deduce_simd<::cat::uint8, 2u>;                             \
   using uint8x4 = deduce_simd<::cat::uint8, 4u>;                             \
   using int_unalign_8x2 = deduce_unaligned_simd<::cat::int8, 2u>;            \
   using int_unalign_8x4 = deduce_unaligned_simd<::cat::int8, 4u>;            \
   using uint_unalign_8x2 = deduce_unaligned_simd<::cat::uint8, 2u>;          \
   using uint_unalign_8x4 = deduce_unaligned_simd<::cat::uint8, 4u>;          \
                                                                              \
   /* float4 / float8 vectors: */                                             \
   using float4x2 = deduce_simd<::cat::float4, 2u>;                           \
   using float4x4 = deduce_simd<::cat::float4, 4u>;                           \
   using float4x8 = deduce_simd<::cat::float4, 8u>;                           \
   using float8x2 = deduce_simd<::cat::float8, 2u>;                           \
   using float8x4 = deduce_simd<::cat::float8, 4u>;                           \
   using float_unalign_4x2 = deduce_unaligned_simd<::cat::float4, 2u>;        \
   using float_unalign_4x4 = deduce_unaligned_simd<::cat::float4, 4u>;        \
   using float_unalign_4x8 = deduce_unaligned_simd<::cat::float4, 8u>;        \
   using float_unalign_8x2 = deduce_unaligned_simd<::cat::float8, 2u>;        \
   using float_unalign_8x4 = deduce_unaligned_simd<::cat::float8, 4u>;        \
                                                                              \
   /* float4_fast / float8_fast vectors: */                                   \
   using float4_fastx2 = deduce_simd<::cat::float4_fast, 2u>;                 \
   using float4_fastx4 = deduce_simd<::cat::float4_fast, 4u>;                 \
   using float4_fastx8 = deduce_simd<::cat::float4_fast, 8u>;                 \
   using float8_fastx2 = deduce_simd<::cat::float8_fast, 2u>;                 \
   using float8_fastx4 = deduce_simd<::cat::float8_fast, 4u>;                 \
   using float_fast_unalign_4x2 =                                             \
      deduce_unaligned_simd<::cat::float4_fast, 2u>;                          \
   using float_fast_unalign_4x4 =                                             \
      deduce_unaligned_simd<::cat::float4_fast, 4u>;                          \
   using float_fast_unalign_4x8 =                                             \
      deduce_unaligned_simd<::cat::float4_fast, 8u>;                          \
   using float_fast_unalign_8x2 =                                             \
      deduce_unaligned_simd<::cat::float8_fast, 2u>;                          \
   using float_fast_unalign_8x4 = deduce_unaligned_simd<::cat::float8_fast, 4u>;
#pragma clang final(CAT_SIMD_ALIASES)

// `CAT_SIMD_PLACEHOLDER_ALIASES(ABI)` allows users to declare convenient
// per-ABI deducing vectors inside a cat::detail::simd_[ABI] namespace for use
// within `$simd_switch` bodies.
#define CAT_SIMD_PLACEHOLDER_ALIASES(ABI)                                    \
   /* int1 / uint1 vectors: */                                               \
   using int1x_ = native_simd<::cat::int1>;                                  \
   using uint1x_ = native_simd<::cat::uint1>;                                \
   using int_unalign_1x_ = native_unaligned_simd<::cat::int1>;               \
   using uint_unalign_1x_ = native_unaligned_simd<::cat::uint1>;             \
   /* char. */                                                               \
   using char1x_ = native_simd<char>;                                        \
   using char_unalign_1x_ = native_unaligned_simd<char>;                     \
   /* int2 / uint2 vectors: */                                               \
   using int2x_ = native_simd<::cat::int2>;                                  \
   using uint2x_ = native_simd<::cat::uint2>;                                \
   using int_unalign_2x_ = native_unaligned_simd<::cat::int2>;               \
   using uint_unalign_2x_ = native_unaligned_simd<::cat::uint2>;             \
   /* int4 / uint4 vectors: */                                               \
   using int4x_ = native_simd<::cat::int4>;                                  \
   using uint4x_ = native_simd<::cat::uint4>;                                \
   using int_unalign_4x_ = native_unaligned_simd<::cat::int4>;               \
   using uint_unalign_4x_ = native_unaligned_simd<::cat::uint4>;             \
   /* int8 / uint8 vectors: */                                               \
   using int8x_ = native_simd<::cat::int8>;                                  \
   using uint8x_ = native_simd<::cat::uint8>;                                \
   using int_unalign_8x_ = native_unaligned_simd<::cat::int8>;               \
   using uint_unalign_8x_ = native_unaligned_simd<::cat::uint8>;             \
   /* float4 / float8 vectors: */                                            \
   using float4x_ = native_simd<::cat::float4>;                              \
   using float8x_ = native_simd<::cat::float8>;                              \
   using float_unalign_4x_ = native_unaligned_simd<::cat::float4>;           \
   using float_unalign_8x_ = native_unaligned_simd<::cat::float8>;           \
   /* float4_fast / float8_fast vectors: */                                  \
   using float4_fastx_ = native_simd<::cat::float4_fast>;                    \
   using float8_fastx_ = native_simd<::cat::float8_fast>;                    \
   using float_fast_unalign_4x_ = native_unaligned_simd<::cat::float4_fast>; \
   using float_fast_unalign_8x_ = native_unaligned_simd<::cat::float8_fast>;
// NOLINTEND
#pragma clang final(CAT_SIMD_PLACEHOLDER_ALIASES)

inline namespace arithmetic {

CAT_SIMD_ALIASES(::cat::simd_abi::native)

}  // namespace arithmetic

// TODO: Add Clang `__bf16` vectors.

namespace detail::simd_sse2 {

CAT_SIMD_ALIASES(::x64::sse_abi)
CAT_SIMD_PLACEHOLDER_ALIASES(::x64::sse_abi)

}  // namespace detail::simd_sse2

namespace detail::simd_ssse3 {

// For now, we use the SSE2 ABI within SSSE3 code.
using namespace simd_sse2;

}  // namespace detail::simd_ssse3

namespace detail::simd_sse4_1 {

// For now, we use the SSE2 ABI within SSE4.1 code.
using namespace simd_sse2;

}  // namespace detail::simd_sse4_1

namespace detail::simd_sse4_2 {

// For now, we use the SSE2 ABI within SSE4.2 code.
using namespace simd_sse2;

}  // namespace detail::simd_sse4_2

namespace detail::simd_avx {

// For now, we use the SSE2 ABI within AVX code.
using namespace simd_sse2;

}  // namespace detail::simd_avx

namespace detail::simd_avx2 {

CAT_SIMD_ALIASES(::x64::avx_abi)
CAT_SIMD_PLACEHOLDER_ALIASES(::x64::avx_abi)

}  // namespace detail::simd_avx2
