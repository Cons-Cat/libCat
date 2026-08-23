#include <cat/bit>
#include <cat/simd>
#include <cat/simd_switch>

namespace cat::detail {

namespace {

// Count set bits in a byte buffer with a scalar loop.
[[nodiscard]]
auto
popcount_bytes_scalar(byte const* _Nonnull p_bytes, idx bytes) -> idx {
   idx count = 0u;
   for (idx byte_index = 0u; byte_index < bytes; ++byte_index) {
      count += popcount(p_bytes[byte_index]);
   }
   return count;
}

// Count set bits in a word buffer with a scalar loop and a masked tail word.
[[nodiscard]]
auto
popcount_words_scalar(uword const* _Nonnull p_words, idx words, uword tail_mask)
   -> idx {
   idx count = 0u;
   idx const last_word = idx(words - 1u);
   for (idx word_index = 0u; word_index < last_word; ++word_index) {
      count += popcount(p_words[word_index]);
   }
   count += popcount(p_words[last_word] & tail_mask);
   return count;
}

// Count set bits in an unmasked word buffer using a CSA SIMD accumulator.
template <typename Simd>
   requires(is_same<typename Simd::memory_lane, uword::raw_type>)
[[nodiscard]]
auto
popcount_words_simd_unmasked(uword const* _Nonnull p_words, idx words) -> idx {
   using lane = Simd::memory_lane;

   lane const* _Nonnull const p_lanes = reinterpret_cast<lane const*>(p_words);
   idx const vector_words = Simd::size();
   idx const block_words = vector_words * 16u;

   // Combine three bit-sliced accumulators into low and carry planes.
   auto carry_save_adder = [](Simd& high, Simd& low, Simd first,
                              Simd second, Simd third) {
      Simd const first_second_xor = first ^ second;
      high = (first & second) | (first_second_xor & third);
      low = first_second_xor ^ third;
   };

   Simd count{};
   Simd ones{};
   Simd twos{};
   Simd fours{};
   Simd eights{};
   Simd sixteens{};
   Simd twos_a;
   Simd twos_b;
   Simd fours_a;
   Simd fours_b;
   Simd eights_a;
   Simd eights_b;

   idx word_index = 0u;
   for (; word_index + block_words <= words; word_index += block_words) {
      carry_save_adder(
         twos_a, ones, ones,
         make_simd_loaded<Simd>(
            p_lanes + word_index + vector_words * 0u
         ),
         make_simd_loaded<Simd>(
            p_lanes + word_index + vector_words * 1u
         )
      );
      carry_save_adder(
         twos_b, ones, ones,
         make_simd_loaded<Simd>(
            p_lanes + word_index + vector_words * 2u
         ),
         make_simd_loaded<Simd>(
            p_lanes + word_index + vector_words * 3u
         )
      );
      carry_save_adder(fours_a, twos, twos, twos_a, twos_b);
      carry_save_adder(
         twos_a, ones, ones,
         make_simd_loaded<Simd>(
            p_lanes + word_index + vector_words * 4u
         ),
         make_simd_loaded<Simd>(
            p_lanes + word_index + vector_words * 5u
         )
      );
      carry_save_adder(
         twos_b, ones, ones,
         make_simd_loaded<Simd>(
            p_lanes + word_index + vector_words * 6u
         ),
         make_simd_loaded<Simd>(
            p_lanes + word_index + vector_words * 7u
         )
      );
      carry_save_adder(fours_b, twos, twos, twos_a, twos_b);
      carry_save_adder(eights_a, fours, fours, fours_a, fours_b);
      carry_save_adder(
         twos_a, ones, ones,
         make_simd_loaded<Simd>(
            p_lanes + word_index + vector_words * 8u
         ),
         make_simd_loaded<Simd>(
            p_lanes + word_index + vector_words * 9u
         )
      );
      carry_save_adder(
         twos_b, ones, ones,
         make_simd_loaded<Simd>(
            p_lanes + word_index + vector_words * 10u
         ),
         make_simd_loaded<Simd>(
            p_lanes + word_index + vector_words * 11u
         )
      );
      carry_save_adder(fours_a, twos, twos, twos_a, twos_b);
      carry_save_adder(
         twos_a, ones, ones,
         make_simd_loaded<Simd>(
            p_lanes + word_index + vector_words * 12u
         ),
         make_simd_loaded<Simd>(
            p_lanes + word_index + vector_words * 13u
         )
      );
      carry_save_adder(
         twos_b, ones, ones,
         make_simd_loaded<Simd>(
            p_lanes + word_index + vector_words * 14u
         ),
         make_simd_loaded<Simd>(
            p_lanes + word_index + vector_words * 15u
         )
      );
      carry_save_adder(fours_b, twos, twos, twos_a, twos_b);
      carry_save_adder(eights_b, fours, fours, fours_a, fours_b);
      carry_save_adder(sixteens, eights, eights, eights_a, eights_b);

      count += sixteens.popcount();
   }

   Simd total = (count * Simd(16u)) + (eights.popcount() * Simd(8u))
                  + (fours.popcount() * Simd(4u))
                  + (twos.popcount() * Simd(2u)) + ones.popcount();

   for (; word_index + vector_words <= words; word_index += vector_words) {
      total +=
         make_simd_loaded<Simd>(p_lanes + word_index).popcount();
   }

   idx count_scalar = idx(total.sum());
   for (; word_index < words; ++word_index) {
      count_scalar += popcount(p_words[word_index]);
   }
   return count_scalar;
}

// Count set bits in a byte buffer by using word SIMD where possible.
template <typename Simd>
[[nodiscard]]
auto
popcount_bytes_simd(byte const* _Nonnull p_bytes, idx bytes) -> idx {
   idx const full_words = bytes / sizeof(uword);
   idx count = popcount_words_simd_unmasked<Simd>(
      reinterpret_cast<uword const*>(p_bytes), full_words
   );
   for (idx byte_index = full_words * sizeof(uword); byte_index < bytes;
        ++byte_index) {
      count += popcount(p_bytes[byte_index]);
   }
   return count;
}

// Count set bits in a word buffer using SIMD and a final tail mask.
template <typename Simd>
[[nodiscard]]
auto
popcount_words_simd(uword const* _Nonnull p_words, idx words, uword tail_mask)
   -> idx {
   if (tail_mask == uword::max()) {
      return popcount_words_simd_unmasked<Simd>(p_words, words);
   }
   idx const full_words = idx(words - 1u);
   return popcount_words_simd_unmasked<Simd>(p_words, full_words)
          + popcount(p_words[full_words] & tail_mask);
}

}  // namespace

// Count set bits in a byte buffer with runtime SIMD dispatch.
auto
popcount_bytes_runtime(byte const* _Nonnull p_bytes, idx bytes) -> idx {
   if (bytes < 512u) {
      return popcount_bytes_scalar(p_bytes, bytes);
   }

   return $simd_switch(
      $abi(
         (avx512, sse2),
         { return popcount_bytes_simd<uint8x_>(p_bytes, bytes); }
      ),
      $abi(
         avx2,
         {
            idx count = popcount_bytes_simd<uint8x_>(p_bytes, bytes);
            x64::zero_upper_avx_registers();
            return count;
         }
      ),
      default : return popcount_bytes_scalar(p_bytes, bytes);
   );
}

// Count set bits in a word buffer with runtime SIMD dispatch.
auto
popcount_words_runtime(
   uword const* _Nonnull p_words, idx words, uword tail_mask
) -> idx {
   if (words < 64u) {
      return popcount_words_scalar(p_words, words, tail_mask);
   }

   return $simd_switch(
      $abi(
         (avx512, sse2),
         { return popcount_words_simd<uint8x_>(p_words, words, tail_mask); }
      ),
      $abi(avx2, {
         idx count = popcount_words_simd<uint8x_>(p_words, words, tail_mask);
         x64::zero_upper_avx_registers();
         return count;
      })
   );
}

}  // namespace cat::detail
