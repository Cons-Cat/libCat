// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/cpuid>
#include <cat/maybe>

namespace x64 {

// https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html#text=_rdrand64_step
// Evaluate an `rdrand` instruction if possible. Note that this is not generally
// considered secure random, because it is statistically flawed on many
// platforms.
//
// https://board.flatassembler.net/topic.php?t=24261
// https://github.com/systemd/systemd/issues/11810#issuecomment-489727505
[[nodiscard, gnu::always_inline]]
inline auto
rdrand() noexcept(!__has_feature(cxx_exceptions)) -> cat::maybe<cat::uint8> {
   if ((cpuid(1u)[2] & (1u << 30u)) == 0u) {
      return cat::nullopt;
   }

   cat::uint8 value;
   bool succeeded;
   asm volatile(R"(rdrand %0
                   setc %1)"
                : "=r"(value), "=qm"(succeeded));
   if (!succeeded) {
      return cat::nullopt;
   }
   return value;
}

class rdrand_engine {
 public:
   using result_type = cat::uint8;

   [[nodiscard]]
   auto
   operator()() const -> result_type {
      return rdrand().assert();
   }

   [[nodiscard]]
   static constexpr auto
   min() noexcept -> result_type {
      return 0u;
   }

   // https://www.felixcloutier.com/x86/rdrand
   [[nodiscard]]
   static constexpr auto
   max() noexcept -> result_type {
      return result_type::max();
   }
};

}  // namespace x64
