// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/array>
#include <cat/bit>
#include <cat/limits>

// Jump polynomials r(x) = x^J mod c(x) over GF(2), after nessan/xoshiro.
// c(x) = x^n + p(x). p(x) is packed into `word_count` words.

namespace cat::detail {

template <typename Raw>
constexpr void
xoshiro_riffle_word(Raw source, Raw& low, Raw& high) {
   constexpr idx bits = sizeof(Raw) * 8u;
   constexpr idx half = bits / 2u;
   Raw const ones = ~Raw(0);
   Raw const one = 1;
   low = source & (ones >> half);
   high = source >> half;
   for (idx width = bits / 4u; width > 0u; width = width / 2u) {
      Raw const split = Raw(one << width) | one;
      Raw const mask = ones / split;
      low = (low ^ (low << width)) & mask;
      high = (high ^ (high << width)) & mask;
   }
}

template <typename Raw, idx word_count>
constexpr void
xoshiro_riffle(
   array<Raw, word_count> const& source, array<Raw, word_count>& low,
   array<Raw, word_count>& high
) {
   Raw even = 0;
   Raw odd = 0;
   for (idx offset = 0u; offset < word_count; ++offset) {
      idx const word = idx(word_count - 1u - offset);
      xoshiro_riffle_word(source[word], even, odd);
      idx const twice = word * 2u;
      if (twice + 1u > word_count) {
         high[idx(twice - word_count)] = even;
         high[idx(twice + 1u - word_count)] = odd;
      } else if (twice + 1u == word_count) {
         low[idx(word_count - 1u)] = even;
         high[0u] = odd;
      } else {
         low[twice] = even;
         low[twice + 1u] = odd;
      }
   }
}

template <typename Raw, idx word_count>
constexpr auto
xoshiro_jump_polynomial(
   array<Raw, word_count> const& characteristic, uword distance,
   bool distance_is_log2
) -> array<Raw, word_count> {
   constexpr idx bits_per_word = sizeof(Raw) * 8u;
   constexpr idx bit_count = word_count * bits_per_word;
   constexpr idx npos = limits<idx>::max();
   Raw const one = 1;

   auto const word_of = [&](idx bit) {
      return bit / bits_per_word;
   };
   auto const mask_of = [&](idx bit) {
      return Raw(one << (bit % bits_per_word));
   };
   auto const test = [&](array<Raw, word_count> const& poly, idx bit) {
      return (poly[word_of(bit)] & mask_of(bit)) != 0;
   };
   auto const set_bit = [&](array<Raw, word_count>& poly, idx bit) {
      poly[word_of(bit)] |= mask_of(bit);
   };
   auto const first_set = [&](array<Raw, word_count> const& poly) {
      for (idx word = 0u; word < word_count; ++word) {
         if (poly[word] != 0) {
            return word * bits_per_word + idx(countr_zero(poly[word]));
         }
      }
      return npos;
   };
   auto const final_set = [&](array<Raw, word_count> const& poly) {
      for (idx offset = 0u; offset < word_count; ++offset) {
         idx const index = idx(word_count - 1u - offset);
         if (poly[index] != 0) {
            return index * bits_per_word + idx(bit_width(poly[index]) - 1u);
         }
      }
      return npos;
   };
   auto const is_monic = [&](array<Raw, word_count> const& poly) {
      Raw const high_bit = Raw(one << (bits_per_word - 1u));
      return (poly[idx(word_count - 1u)] & high_bit) != 0;
   };
   auto const add =
      [&](array<Raw, word_count>& left, array<Raw, word_count> const& right) {
         for (idx word = 0u; word < word_count; ++word) {
            left[word] ^= right[word];
         }
      };
   auto const shift = [&](array<Raw, word_count>& poly) {
      auto const top = bits_per_word - 1u;
      for (idx offset = 1u; offset < word_count; ++offset) {
         idx const word = idx(word_count - offset);
         idx const previous = idx(word - 1u);
         Raw const high = Raw(poly[word] << 1u);
         Raw const low = Raw(poly[previous] >> top);
         poly[word] = high | low;
      }
      poly[0u] = Raw(poly[0u] << 1u);
   };
   auto const times_x = [&](array<Raw, word_count>& poly) {
      bool const reduce_by_p = is_monic(poly);
      shift(poly);
      if (reduce_by_p) {
         add(poly, characteristic);
      }
   };

   array<array<Raw, word_count>, bit_count> power_mod;
   power_mod[0u] = characteristic;
   for (idx power = 1u; power < bit_count; ++power) {
      power_mod[power] = power_mod[idx(power - 1u)];
      times_x(power_mod[power]);
   }

   array<Raw, word_count> high;
   auto const square = [&](array<Raw, word_count>& poly) {
      xoshiro_riffle(poly, poly, high);
      idx const first = first_set(high);
      if (first == npos) {
         return;
      }
      idx const last = final_set(high);
      for (idx bit = first; bit <= last; bit = bit + 2u) {
         if (test(high, bit)) {
            add(poly, power_mod[bit]);
         }
      }
   };

   array<Raw, word_count> result;
   result.fill(0);

   if (distance_is_log2) {
      set_bit(result, 1u);
      for (uword step = 0u; step < distance; ++step) {
         square(result);
      }
      return result;
   }

   if (distance < uword(bit_count)) {
      set_bit(result, idx(distance));
      return result;
   }
   if (distance == uword(bit_count)) {
      return characteristic;
   }

   uword bit = bit_floor(distance);
   set_bit(result, 1u);
   bit >>= 1u;
   while (bit != 0u) {
      square(result);
      if ((distance & bit) != 0u) {
         times_x(result);
      }
      bit >>= 1u;
   }
   return result;
}

}  // namespace cat::detail
