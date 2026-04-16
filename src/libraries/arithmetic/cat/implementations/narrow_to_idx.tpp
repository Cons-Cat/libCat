// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// Included at the end of `<cat/maybe>` after the `maybe` primary template is
// complete. Do not `#include <cat/maybe>` from this file.

#include <cat/arithmetic>

namespace cat {

[[nodiscard]] constexpr auto
narrow_to_idx(idx value) -> maybe<idx> {
   return value;
}

template <is_integral T>
   requires(sizeof(T) == 8)
[[nodiscard]] constexpr auto
narrow_to_idx(T value) -> maybe<idx> {
   __SIZE_TYPE__ wide = make_raw_arithmetic(value);
   if ((wide & limits<idx>::high_bit) != 0u) {
      return nullopt;
   }
   return idx(wide);
}


template <>
[[nodiscard]] constexpr auto
arithmetic<make_signed_type<__SIZE_TYPE__>, overflow_policies::undefined>::
   to_idx() const -> maybe<index<overflow_policies::undefined>> {
   return narrow_to_idx(iword(*this));
}

template <>
[[nodiscard]] constexpr auto
arithmetic<__SIZE_TYPE__, overflow_policies::undefined>::to_idx() const
   -> maybe<index<overflow_policies::undefined>> {
   return narrow_to_idx(uword(*this));
}

}
