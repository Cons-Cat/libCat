// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/bit>
#include <cat/bitset>
#include <cat/simd>

namespace cat::detail {

template <typename Vector>
[[nodiscard, gnu::always_inline]]
inline auto
compare_memory_mismatch_order(
   Vector const& left_vec, Vector const& right_vec, char const* _Nonnull p_left,
   char const* _Nonnull p_right
) -> std::strong_ordering {
   auto const equal_mask = left_vec.equal_lanes(right_vec);
   idx const byte_offset = equal_mask.to_bitset().countr_one();

   return static_cast<unsigned char>(p_left[byte_offset])
          <=> static_cast<unsigned char>(p_right[byte_offset]);
}

template <typename Vector>
[[nodiscard, gnu::always_inline]]
inline auto
compare_memory_compare_chunk(
   char const* _Nonnull p_left, char const* _Nonnull p_right
) -> std::strong_ordering {
   Vector left_vec;
   Vector right_vec;
   left_vec.load_unaligned(p_left);
   right_vec.load_unaligned(p_right);

   if (left_vec.equal_lanes(right_vec).all_of()) {
      return std::strong_ordering::equal;
   }

   return compare_memory_mismatch_order(left_vec, right_vec, p_left, p_right);
}

}  // namespace cat::detail
