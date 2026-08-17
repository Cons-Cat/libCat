// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/debug>
#include <cat/linux>
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
      // Stream random bytes into `result` one byte at a time.
      scaredy_nix<cat::idx> read_result =
         sys_getrandom(p_output + filled, sizeof(result) - filled, flags);

      if (read_result.is_empty()) {
         if (read_result.error() == linux_error::intr) {
            continue;
         }

         read_result.assert();
      }

      cat::idx const count = read_result.value();
      cat::assert(count > 0u);
      filled += count;
   }

   return result;
}

// https://man7.org/linux/man-pages/man4/random.4.html
[[nodiscard]]
inline auto
kernel_random_file_uint8(cat::zstr_view path) -> cat::uint8 {
   file_descriptor const descriptor =
      sys_open(path, open_mode::read_only).assert();
   $defer {
      auto _ = sys_close(descriptor);
   };

   cat::uint8 result;
   char* p_output = reinterpret_cast<char*>(&result);
   cat::idx filled = 0u;

   while (filled < sizeof(result)) {
      scaredy_nix<cat::idx> read_result =
         sys_read(descriptor, p_output + filled, sizeof(result) - filled);

      if (read_result.is_empty()) {
         if (read_result.error() == linux_error::intr) {
            continue;
         }

         read_result.assert();
      }

      cat::idx const count = read_result.value();
      cat::assert(count > 0u);
      filled += count;
   }

   return result;
}

}  // namespace nix::detail

namespace nix {

template <getrandom_flags flags>
class linux_getrandom_engine {
 public:
   using result_type = cat::uint8;

   [[nodiscard]]
   auto
   operator()() const -> result_type {
      return detail::kernel_random_uint8(flags);
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

using sys_urandom_engine = linux_getrandom_engine<getrandom_flags::none>;
using sys_random_engine = linux_getrandom_engine<getrandom_flags::random>;

enum class linux_random_file : cat::uint1::raw_type {
   urandom,
   random,
};

template <linux_random_file file>
class linux_random_file_engine {
 public:
   using result_type = cat::uint8;

   [[nodiscard]]
   auto
   operator()() const -> result_type {
      if constexpr (file == linux_random_file::urandom) {
         return detail::kernel_random_file_uint8("/dev/urandom");
      } else {
         return detail::kernel_random_file_uint8("/dev/random");
      }
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

using dev_urandom_engine = linux_random_file_engine<linux_random_file::urandom>;
using dev_random_engine = linux_random_file_engine<linux_random_file::random>;

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
      .kernel_random = sys_urandom_engine{}(),
      .process = sys_getpid(),
      .thread = sys_gettid(),
      .cycles = x64::read_timestamp_counter(),
      .temperature = cpu_temperature(),
   };
}

}  // namespace nix
