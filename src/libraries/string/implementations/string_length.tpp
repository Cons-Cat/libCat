// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

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
      constexpr x64::sse2_unaligned_simd<char> zeros = '\0';

      for (idx i;; i += 16u) {
         x64::sse2_unaligned_simd<char> const data =
            make_simd_loaded<x64::sse2_unaligned_simd<char>>(p_string + i);
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
