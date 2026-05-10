// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/array>
#include <cat/simd>
#include <cat/string>
#include <cat/utility>

// This function requires SSE4.2, unless it is used in a `constexpr` context.
constexpr auto
cat::string_length(char const* p_string) -> idx {
   if consteval {
      idx result;
      while (true) {
         if (p_string[result.raw] == '\0') {
            return result;
         }
         result++;
      }
   } else {
      // TODO: Implement with portable SIMD, and tune performance.
      constexpr x64::sse_unaligned_simd<char> zeros = '\0';

      for (idx i;; i += 16u) {
         x64::sse_unaligned_simd<char> const data =
            make_simd_loaded<x64::sse_unaligned_simd<char>>(p_string + i);
         constexpr x64::string_control mask =
            x64::string_control::unsigned_byte
            | x64::string_control::compare_equal_each
            | x64::string_control::least_significant;

         // If there are one or more 0 bytes in `data`:
         if (x64::compare_implicit_length_strings<mask>(data, zeros)) {
            uint4 const index =
               x64::compare_implicit_length_strings_return_index<mask>(data,
                                                                       zeros);
            // `index` is the in-chunk offset of the first 0 from `pcmpistri`.
            // `i + index` is that byte's offset from `p_string`, which is the
            // count of `char` before the terminator.
            return i + index;
         }
      }

      __builtin_unreachable();
   }
}

inline auto
cat::detail::compare_strings_detail(str_view const string_1,
                                    str_view const string_2) -> bool {
   if (string_1.size() != string_2.size()) {
      return false;
   }

   using vector = char1x16;

   array<vector, 4u> vectors_1;
   array<vector, 4u> vectors_2;
   array<vector::mask_type, 4u> comparisons;
   idx length_iterator = string_1.size();
   uword vector_size = sizeof(vector);
   char const* p_string_1_iterator = string_1.data();
   char const* p_string_2_iterator = string_2.data();

   auto loop = [&](idx batch_count) -> bool {
      while (length_iterator >= vector_size * batch_count) {
         for (idx i = 0u; i < batch_count; ++i) {
            char const* const p_chunk_1 =
               p_string_1_iterator + (i.raw * vector_size.raw);
            char const* const p_chunk_2 =
               p_string_2_iterator + (i.raw * vector_size.raw);
            vectors_1[i].load_unaligned(p_chunk_1);
            vectors_2[i].load_unaligned(p_chunk_2);
            comparisons[i] = vectors_1[i].equal_lanes(vectors_2[i]);
         }

         for (idx i = 0u; i < batch_count; ++i) {
            // If any lanes are not equal to each other:
            if (!comparisons[i].all_of()) {
               return false;
            }
         }

         length_iterator.raw -= vector_size.raw * batch_count.raw;
         p_string_1_iterator += vector_size.raw * batch_count.raw;
         p_string_2_iterator += vector_size.raw * batch_count.raw;
      }

      return true;
   };

   // Compare four, two, then one vectors of characters at a time.
   if (!loop(4_idx)) {
      return false;
   }
   if (!loop(2_idx)) {
      return false;
   }
   if (!loop(1_idx)) {
      return false;
   }

   // Compare remaining characters individually.
   for (idx i = 0u; i < length_iterator;
        ++i, ++p_string_1_iterator, ++p_string_2_iterator) {
      if (*p_string_1_iterator != *p_string_2_iterator) {
         return false;
      }
   }

   return true;
}
