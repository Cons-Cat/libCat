// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/linux>
#include <cat/random>
#include <cat/timer>

namespace nix::detail {

// https://man7.org/linux/man-pages/man2/getrandom.2.html
[[nodiscard]]
inline auto
kernel_random_uint8(getrandom_flags flags) -> cat::uint8 {
   cat::uint8 result;
   auto* p_output = reinterpret_cast<unsigned char*>(&result);
   cat::idx filled = 0u;

   while (filled < sizeof(result)) {
      scaredy_nix<cat::idx> read_result =
         sys_getrandom(p_output + filled, sizeof(result) - filled, flags);
      if (read_result.is_empty()) {
         if (read_result.error() == linux_error::intr) {
            continue;
         }
         read_result.verify();
      }

      cat::idx const count = read_result.value();
      cat::verify(count > 0u);
      filled += count;
   }
   return result;
}

}  // namespace nix::detail

namespace nix {

class linux_urandom_engine {
 public:
   using result_type = cat::uint8;

   [[nodiscard]]
   auto
   operator()() const -> result_type {
      return detail::kernel_random_uint8(getrandom_flags::none);
   }

   [[nodiscard]]
   static constexpr auto
   min() -> result_type {
      return 0u;
   }

   [[nodiscard]]
   static constexpr auto
   max() -> result_type {
      return result_type::max();
   }
};

class linux_random_engine {
 public:
   using result_type = cat::uint8;

   [[nodiscard]]
   auto
   operator()() const -> result_type {
      return detail::kernel_random_uint8(getrandom_flags::random);
   }

   [[nodiscard]]
   static constexpr auto
   min() -> result_type {
      return 0u;
   }

   [[nodiscard]]
   static constexpr auto
   max() -> result_type {
      return result_type::max();
   }
};

struct seed_state {
   cat::uint8 kernel_random;
   process_id process;
   process_id thread;
   cat::uint8 cycles;
   cat::maybe<cat::int4> temperature;
};

[[nodiscard]]
inline auto
make_seed_state() -> seed_state {
   // Timing, identity, and temperature are diversity only, not entropy.
   return {
      .kernel_random = linux_urandom_engine{}(),
      .process = sys_getpid(),
      .thread = sys_gettid(),
      .cycles = x64::read_timestamp_counter(),
      .temperature = cpu_temperature(),
   };
}

}  // namespace nix
