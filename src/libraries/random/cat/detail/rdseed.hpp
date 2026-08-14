// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/cpuid>
#include <cat/maybe>

namespace x64 {

// https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html#text=_rdseed64_step
[[nodiscard, gnu::always_inline]]
inline auto
rdseed() -> cat::maybe<cat::uint8> {
   auto const maximum_leaf = cpuid(0u)[0];
   if (maximum_leaf < 7u || (cpuid(7u)[1] & (1u << 18u)) == 0u) {
      return cat::nullopt;
   }

   cat::uint8 value;
   unsigned char succeeded;
   asm volatile("rdseed %0\n\tsetc %1"
                : "=r"(value), "=qm"(succeeded));
   if (succeeded == 0u) {
      return cat::nullopt;
   }
   return value;
}

}  // namespace x64
