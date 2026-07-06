#include <cat/arithmetic>
#include <cat/array>
#include <cat/memory>
#include <cat/simd>
#include <cat/simd_dispatch>

#include "compare_memory_avx2.tpp"
#include "compare_memory_medium.tpp"
#include "compare_memory_small.tpp"

namespace cat::detail {

template <typename Vector>
[[nodiscard]]
inline auto
compare_memory_mismatch_in_chunk(
   Vector const& left_vec, Vector const& right_vec,
   char const* _Nonnull p_chunk_left, char const* _Nonnull p_chunk_right
) -> maybe<std::strong_ordering> {
   auto const equal_mask = left_vec.equal_lanes(right_vec);
   if (equal_mask.all_of()) {
      return nullopt;
   }

   return compare_memory_mismatch_order(
      left_vec, right_vec, p_chunk_left, p_chunk_right
   );
}

template <typename Vector>
[[nodiscard]]
inline auto
compare_memory_batch_all_equal(
   array<typename Vector::mask_type, 4u> const& equal_masks
) -> bool {
   if constexpr (sizeof(Vector) == 32u) {
      __UINT32_TYPE__ const full_mask =
         ::x64::detail::avx2_movmsk_full_lane_mask(sizeof(Vector));
      __UINT32_TYPE__ const differing_lanes =
         (x64::detail::avx2_abi_mask_to_bitset(equal_masks[0]) ^ full_mask)
         | (x64::detail::avx2_abi_mask_to_bitset(equal_masks[1]) ^ full_mask)
         | (x64::detail::avx2_abi_mask_to_bitset(equal_masks[2]) ^ full_mask)
         | (x64::detail::avx2_abi_mask_to_bitset(equal_masks[3]) ^ full_mask);
      return differing_lanes == 0u;
   }

   return equal_masks[0].all_of() && equal_masks[1].all_of()
          && equal_masks[2].all_of() && equal_masks[3].all_of();
}

template <typename Vector>
[[nodiscard]]
inline auto
compare_memory_first_mismatch_in_batch(
   array<Vector, 4u> const& vectors_left,
   array<Vector, 4u> const& vectors_right,
   array<typename Vector::mask_type, 4u> const& equal_masks,
   char const* _Nonnull p_left, char const* _Nonnull p_right, uword vector_size
) -> std::strong_ordering {
#pragma unroll 4
   for (idx vector_index = 0u; vector_index < 4u; ++vector_index) {
      if (!equal_masks[vector_index].all_of()) {
         char const* _Nonnull const p_chunk_left =
            p_left + (vector_index * vector_size);
         char const* _Nonnull const p_chunk_right =
            p_right + (vector_index * vector_size);

         if (
            auto result = compare_memory_mismatch_in_chunk(
               vectors_left[vector_index], vectors_right[vector_index],
               p_chunk_left, p_chunk_right
            );
            result.has_value()
         ) {
            return result.value();
         }
      }
   }
   return std::strong_ordering::equal;
}

template <typename Vector>
auto
compare_memory_large(
   byte const* _Nonnull p_lhs, byte const* _Nonnull p_rhs, idx bytes
) -> std::strong_ordering {
   char const* _Nonnull p_left = reinterpret_cast<char const* _Nonnull>(p_lhs);
   char const* _Nonnull p_right = reinterpret_cast<char const* _Nonnull>(p_rhs);

   constexpr uword vector_size = sizeof(Vector);
   iword length_iterator = bytes;

   array<Vector, 4u> vectors_left;
   array<Vector, 4u> vectors_right;
   array<typename Vector::mask_type, 4u> equal_masks;

   while (length_iterator >= static_cast<iword>(vector_size * 4u)) {
      for (idx vector_index = 0u; vector_index < 4u; ++vector_index) {
         char const* _Nonnull const p_chunk_left =
            p_left + (vector_index * vector_size);
         char const* _Nonnull const p_chunk_right =
            p_right + (vector_index * vector_size);
         vectors_left[vector_index].load_unaligned(p_chunk_left);
         vectors_right[vector_index].load_unaligned(p_chunk_right);
         equal_masks[vector_index] =
            vectors_left[vector_index].equal_lanes(vectors_right[vector_index]);
      }

      if (compare_memory_batch_all_equal<Vector>(equal_masks)) {
         length_iterator -= vector_size * 4u;
         p_left += vector_size * 4u;
         p_right += vector_size * 4u;
         continue;
      }

      return compare_memory_first_mismatch_in_batch(
         vectors_left, vectors_right, equal_masks, p_left, p_right, vector_size
      );
   }

   while (length_iterator >= static_cast<iword>(vector_size)) {
      Vector left_vec;
      Vector right_vec;
      left_vec.load_unaligned(p_left);
      right_vec.load_unaligned(p_right);

      if (
         auto result = compare_memory_mismatch_in_chunk(
            left_vec, right_vec, p_left, p_right
         );
         result.has_value()
      ) {
         return result.value();
      }

      length_iterator -= vector_size;
      p_left += vector_size;
      p_right += vector_size;
   }

   if (length_iterator > 0) {
      return compare_memory_small(
         reinterpret_cast<byte const* _Nonnull>(p_left),
         reinterpret_cast<byte const* _Nonnull>(p_right),
         length_iterator.to_idx().assert()
      );
   }

   return std::strong_ordering::equal;
}

auto
compare_memory_impl(
   byte const* _Nonnull p_lhs, byte const* _Nonnull p_rhs, idx bytes
) -> std::strong_ordering {
   if (p_lhs == p_rhs) {
      return std::strong_ordering::equal;
   }

   if (bytes <= 16u) {
      return compare_memory_small(p_lhs, p_rhs, bytes);
   }

   if (bytes <= 255u) {
#ifndef CAT_NO_CPUID
      if (simd_dispatch_priority >= 80) {
         return compare_memory_avx2(p_lhs, p_rhs, bytes);
      }
#endif
      if (bytes <= 127u) {
         return compare_memory_medium(p_lhs, p_rhs, bytes);
      }
   }

   return $simd_switch(
      $abi(
         avx2, { return compare_memory_large<char1x_>(p_lhs, p_rhs, bytes); }
      ),
      $abi(sse2, { return compare_memory_large<char1x_>(p_lhs, p_rhs, bytes); })
   );
}

}  // namespace cat::detail
