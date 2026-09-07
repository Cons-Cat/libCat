// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/bit>
#include <cat/maybe>

namespace cat {

template <typename To, is_integral From>
   requires(is_same<To, byte>)
[[nodiscard]]
constexpr auto
narrow_cast(From value) -> maybe<byte> {
   auto const raw = make_raw_arithmetic(value);
   if (
      detail::raw_mixed_integral_spaceship(raw, 0u) < 0
      || detail::raw_mixed_integral_spaceship(raw, 255u) > 0
   ) {
      return nullopt;
   }
   return byte(raw);
}

}  // namespace cat
