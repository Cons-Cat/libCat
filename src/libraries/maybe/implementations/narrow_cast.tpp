// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>
#include <cat/maybe>

// `cat::narrow_cast` is analogous to `gsl::narrow_cast`, but returns a `maybe`
// instead of throwing an exception.

namespace cat {

// `narrow_cast` safely casts one integer to a different integer type,
// safely returning a `maybe` holding `nullopt` when the value is out of range
// of the new type. This should be used instead of `static_cast` for integer
// conversions.
template <is_integral To, is_integral From>
[[nodiscard]]
constexpr auto
narrow_cast(From value) -> maybe<To> {
   if constexpr (is_safe_arithmetic_conversion<From, To>) {
      return To(value);
   } else {
      auto const raw = make_raw_arithmetic(value);
      if (detail::raw_mixed_integral_spaceship(raw, limits<To>::min()) < 0
          || detail::raw_mixed_integral_spaceship(raw, limits<To>::max()) > 0) {
         return nullopt;
      }
      return To(raw);
   }
}

}  // namespace cat
