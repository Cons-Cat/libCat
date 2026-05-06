#pragma once

#include <cat/simd>

#include "cat/detail/simd_abi_hooks.hpp"

namespace cat::detail::simd_abi {

template <x64::is_avx2_abi<float> Abi>
struct unary_full<op_rsqrt, float, Abi> {
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   static constexpr auto
   invoke(simd<float, Abi> input) -> simd<float, Abi> {
      using simd_type = simd<float, Abi>;
      // Hardware `rsqrt` is very imprecise. We correct error with
      // Newton-Raphson, which is still faster than a precise 1/`sqrt(x)`
      // method.
      simd_type const rsqrt = __builtin_ia32_rsqrtps256(input.raw);
      simd_type const minus_half_x = -0.5f * input;
      simd_type const three_halves = 1.5f;
      simd_type const correction = __builtin_elementwise_fma(
         minus_half_x.raw, rsqrt.raw * rsqrt.raw, three_halves.raw);
      return rsqrt * correction;
   }
};

}  // namespace cat::detail::simd_abi
