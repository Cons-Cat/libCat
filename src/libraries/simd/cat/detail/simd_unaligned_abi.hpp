#pragma once

#include <cat/arithmetic>

namespace cat {

namespace simd_abi {
template <typename AbiTag, typename ElementT>
struct mask_lane;
}

// Unaligned view of a base `simd` ABI. Same `size` and `lanes` as `BaseAbi` but
// `alignment` is 1 so `gnu::vector_size` storage is not over-aligned (mirrors
// `x64::avx2_unaligned_abi`/`x64::sse2_unaligned_abi` before the generic
// spelling).
template <typename BaseAbi>
struct unaligned_abi {
   using scalar_type = typename BaseAbi::scalar_type;

   template <typename U>
   using make_abi_type =
      unaligned_abi<typename BaseAbi::template make_abi_type<U>>;

   constexpr unaligned_abi() = delete;

   static constexpr idx size = BaseAbi::size;
   static constexpr idx lanes = BaseAbi::lanes;
   static constexpr uword alignment = 1u;

   template <typename ElementT>
   using simd_mask_lane = simd_abi::mask_lane<
      unaligned_abi<typename BaseAbi::template make_abi_type<ElementT>>,
      ElementT>;
};

}  // namespace cat
