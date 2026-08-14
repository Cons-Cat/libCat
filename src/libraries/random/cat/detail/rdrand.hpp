// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/cpuid>
#include <cat/maybe>

namespace x64 {

// https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html#text=_rdrand64_step
[[nodiscard, gnu::always_inline]]
inline auto
rdrand() -> cat::maybe<cat::uint8> {
   if ((cpuid(1u)[2] & (1u << 30u)) == 0u) {
      return cat::nullopt;
   }

   cat::uint8 value;
   bool succeeded;
   asm volatile(R"(rdrand %0
                   tsetc %1)"
                : "=r"(value), "=qm"(succeeded));
   if (!succeeded) {
      return cat::nullopt;
   }
   return value;
}

}  // namespace x64
