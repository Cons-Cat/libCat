#pragma once

#include <cat/simd>

#include "cat/detail/simd_abi_hooks.hpp"

namespace cat::detail::simd_abi {

template <>
struct unary_full<op_rsqrt, float, x64::sse2_abi<float>> {
   [[nodiscard]]
   static constexpr auto
   invoke(simd<float, x64::sse2_abi<float>> input)
      -> simd<float, x64::sse2_abi<float>> {
      return simd<float, x64::sse2_abi<float>>(
         __builtin_ia32_rsqrtps(input.raw));
   }
};

template <>
struct unary_full<op_rsqrt, float, x64::sse2_unaligned_abi<float>> {
   [[nodiscard]]
   static constexpr auto
   invoke(simd<float, x64::sse2_unaligned_abi<float>> input)
      -> simd<float, x64::sse2_unaligned_abi<float>> {
      return simd<float, x64::sse2_unaligned_abi<float>>(
         __builtin_ia32_rsqrtps(input.raw));
   }
};

}  // namespace cat::detail::simd_abi
