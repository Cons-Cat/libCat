// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/bit>

namespace cat {

template <is_integral auto alignment, typename U>
   requires(alignment > 0u && has_single_bit(alignment))
[[nodiscard, gnu::returns_nonnull]]
constexpr auto
assume_aligned(U* _Nonnull p_value) -> U* _Nonnull {
   if consteval {
      return p_value;
   } else {
      return static_cast<U*>(
         __builtin_assume_aligned(p_value, make_raw_arithmetic(alignment))
      );
   }
}

template <is_integral auto alignment, typename U>
   requires(alignment <= 0u || !has_single_bit(alignment))
[[nodiscard]]
constexpr auto
assume_aligned(U* _Nonnull)
   -> U* _Nonnull = delete ("`alignment` must be a power of two!");

}  // namespace cat
