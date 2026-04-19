#pragma once

// Default `simd_mask` lane image for libCat x64 ABIs. Xmm and ymm slots use
// `pcmpeq`-style storage (one `simd_vector_element` per logical lane, false is
// zero, true is all bits set in the lane). For other encodings, specialize
// `simd_abi::mask_lane<YourAbi, T>`.
//
// Included from `<cat/simd>` after `class simd`. See `simd_mask_lane` on each
// ABI tag in `simd_avx2.hpp`, `simd_sse42.hpp`, and the portable ABIs in
// `<cat/simd>`.

namespace cat::simd_abi {

template <typename AbiTag, typename ElementT>
struct mask_lane {
   using lane_scalar = simd_vector_element<ElementT>;
   using raw_type = typename simd<
      lane_scalar,
      typename AbiTag::template make_abi_type<lane_scalar>>::raw_type;

   static_assert(AbiTag::template make_abi_type<lane_scalar>::lanes
                    == AbiTag::lanes,
                 "`simd_mask` lane count must match its simd ABI");

   // Packed compare storage. Logical false is `lane_scalar{}`. Logical true
   // sets every bit in the lane (`~lane_scalar{}` for integral lanes,
   // `__builtin_bit_cast(lane_scalar, ~bits_t{})` for others). `decode_lane`
   // and `encode_lane` together define truthiness. Reading follows the same
   // rule as writing (`encode_lane(false)` must yield what `decode_lane` reads
   // as false, and likewise for true).
   [[nodiscard]]
   static constexpr auto
   decode_lane(raw_type const& raw, idx i) -> bool {
      lane_scalar const slot = raw[static_cast<unsigned>(i.raw)];
      if constexpr (is_integral<lane_scalar>) {
         return slot != lane_scalar{};
      }
      using bits_t = detail::simd_mask_lane_bits_t<lane_scalar>;
      return __builtin_bit_cast(bits_t, slot) != bits_t{};
   }

   static constexpr void
   encode_lane(raw_type& raw, idx i, bool value) {
      unsigned const lane = static_cast<unsigned>(i.raw);
      if (value) {
         if constexpr (is_integral<lane_scalar>) {
            raw[lane] = ~lane_scalar{};
         } else {
            using bits_t = detail::simd_mask_lane_bits_t<lane_scalar>;
            raw[lane] = __builtin_bit_cast(lane_scalar, ~bits_t{});
         }
      } else {
         raw[lane] = lane_scalar{};
      }
   }
};

}  // namespace cat::simd_abi
