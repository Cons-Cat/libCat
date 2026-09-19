#include <cat/atomic>
#include <cat/chrono>
#include <cat/cpuid>
#include <cat/timer>

namespace {

inline constexpr cat::uint8 fixed_shift = 32u;
inline constexpr cat::int8 maximum_error = 1'000'000;
inline constexpr cat::int8 calibration_interval = 3'000'000'000;

inline constexpr cat::uint4 uninitialized = 0u;
inline constexpr cat::uint4 initializing = 1u;
inline constexpr cat::uint4 ready = 2u;
inline constexpr cat::uint4 unavailable = 3u;

struct synchronized_time {
   cat::uint8 tsc;
   cat::int8 nanoseconds;
};

struct conversion {
   cat::uint8 base_tsc;
   cat::int8 base_nanoseconds;
   cat::uint8 multiplier;
};

struct clock_state {
   cat::atomic<cat::uint4> sequence;
   cat::atomic<cat::uint8> base_tsc;
   cat::atomic<cat::int8> base_nanoseconds;
   cat::atomic<cat::uint8> multiplier;
   cat::atomic<cat::int8> base_error;
   cat::atomic<cat::uint8> next_calibration;
   cat::atomic<cat::uint4> initialized;
   cat::atomic_flag calibrating;
};

constinit clock_state state{};

[[nodiscard]]
auto
read_tsc() -> cat::uint8 {
   cat::uint4 low;
   cat::uint4 high;
   asm volatile(R"(lfence
                   rdtsc)"
                : "=a"(low), "=d"(high)
                :
                : "memory");
   return cat::uint8(high) << 32u | low;
}

[[nodiscard]]
auto
synchronize_time() -> cat::maybe<synchronized_time> {
   synchronized_time best{};
   cat::uint8 best_span = cat::uint8::max();

   for (cat::idx index = 0; index < 5; ++index) {
      cat::uint8 const before = read_tsc();
      nix::timespec kernel_time;
      if (
         nix::sys_clock_gettime(nix::clock_id::monotonic_raw, kernel_time)
            .is_empty()
      ) {
         return cat::nullopt;
      }
      cat::uint8 const after = read_tsc();
      cat::uint8 const span = after - before;
      if (span < best_span) {
         best_span = span;
         best = {
            .tsc = before + span / 2u,
            .nanoseconds = kernel_time.to_nanoseconds().raw,
         };
      }
   }

   return best;
}

[[nodiscard]]
auto
read_conversion() -> conversion {
   for (;;) {
      cat::uint4 const before = state.sequence.acquire();
      if ((before & 1u) != 0u) {
         cat::machine_pause();
         continue;
      }

      conversion const result = {
         .base_tsc = state.base_tsc.relaxed(),
         .base_nanoseconds = state.base_nanoseconds.relaxed(),
         .multiplier = state.multiplier.relaxed(),
      };
      if (before == state.sequence.acquire().load()) {
         return result;
      }
   }
}

void
write_conversion(
   conversion const& value, cat::int8 base_error,
   cat::uint8 next_calibration_value
) {
   cat::uint4 sequence = state.sequence.relaxed();
   state.sequence.seq_cst() = sequence + 1u;
   state.base_tsc.relaxed() = value.base_tsc;
   state.base_nanoseconds.relaxed() = value.base_nanoseconds;
   state.multiplier.relaxed() = value.multiplier;
   state.base_error.relaxed() = base_error;
   state.next_calibration.relaxed() = next_calibration_value;
   state.sequence.seq_cst() = sequence + 2u;
}

[[nodiscard]]
auto
to_nanoseconds(cat::uint8 tsc, conversion const& parameters)
   -> cat::maybe<cat::int8> {
   if (tsc < parameters.base_tsc) {
      return cat::nullopt;
   }
   cat::uint8 const elapsed_tsc = tsc - parameters.base_tsc;
   __uint128_t const elapsed_fixed =
      __uint128_t(elapsed_tsc) * parameters.multiplier.raw;
   return parameters.base_nanoseconds + cat::int8(elapsed_fixed >> fixed_shift);
}

[[nodiscard]]
auto
make_multiplier(cat::int8 elapsed_nanoseconds, cat::uint8 elapsed_tsc)
   -> cat::maybe<cat::uint8> {
   if (elapsed_nanoseconds <= 0 || elapsed_tsc == 0u) {
      return cat::nullopt;
   }
   __uint128_t const fixed = __uint128_t(elapsed_nanoseconds) << fixed_shift;
   cat::uint8 const multiplier = cat::uint8(fixed / elapsed_tsc);
   if (multiplier == 0u) {
      return cat::nullopt;
   }
   return multiplier;
}

[[nodiscard]]
auto
next_calibration(cat::uint8 tsc, cat::uint8 multiplier) -> cat::uint8 {
   __uint128_t const interval_fixed = __uint128_t(calibration_interval)
                                      << fixed_shift;
   return tsc + cat::uint8(interval_fixed / multiplier);
}

[[nodiscard]]
auto
sleep_for_calibration(x64::clock_tsc::duration elapsed) {
   if (elapsed < 1 * cat::units::millisecond) {
      elapsed = 1 * cat::units::millisecond;
   }

   nix::timespec remaining = nix::make_timespec(elapsed);
   while (true) {
      nix::scaredy_nix<void> const result =
         nix::sys_nanosleep(remaining, &remaining);
      if (result.has_value()) {
         return true;
      }

      // Retry the loop if we get an interrupt.
      if (result.error() != nix::linux_error::intr) {
         return false;
      }
   }
}

}  // namespace

auto
x64::clock_tsc::initialize(duration calibration_time) -> bool {
   cat::uint4 initialized = state.initialized.acquire();
   if (initialized == ready) {
      return true;
   }
   if (initialized == unavailable) {
      return false;
   }

   cat::uint4 expected = uninitialized;
   if (
      state.initialized.compare_exchange_strong(
         expected, initializing, cat::memory_order::acq_rel,
         cat::memory_order::acquire
      )
   ) {
      if (!x64::has_invariant_tsc()) {
         state.initialized.release() = unavailable;
         return false;
      }

      cat::maybe<synchronized_time> const first = synchronize_time();
      if (first.is_empty()) {
         state.initialized.release() = unavailable;
         return false;
      }

      if (!sleep_for_calibration(calibration_time)) {
         state.initialized.release() = unavailable;
         return false;
      }

      cat::maybe<synchronized_time> const second = synchronize_time();
      if (second.is_empty() || second.value().tsc <= first.value().tsc) {
         state.initialized.release() = unavailable;
         return false;
      }

      cat::maybe<cat::uint8> const multiplier = make_multiplier(
         second.value().nanoseconds - first.value().nanoseconds,
         second.value().tsc - first.value().tsc
      );

      if (multiplier.is_empty()) {
         state.initialized.release() = unavailable;
         return false;
      }

      write_conversion(
         {
            .base_tsc = first.value().tsc,
            .base_nanoseconds = first.value().nanoseconds,
            .multiplier = multiplier.value(),
         },
         0, next_calibration(second.value().tsc, multiplier.value())
      );

      state.initialized.release() = ready;
      return true;
   }

   while ((initialized = state.initialized.acquire()) == initializing) {
      cat::machine_pause();
   }
   return initialized == ready;
}

auto
x64::clock_tsc::now() -> cat::maybe<time_point> {
   if (!initialize()) {
      return cat::nullopt;
   }

   cat::maybe<cat::int8> const converted =
      to_nanoseconds(read_tsc(), read_conversion());
   if (converted.is_empty()) {
      return cat::nullopt;
   }

   return time_point(converted.value());
}

void
x64::clock_tsc::calibrate() {
   if (
      !initialize()
      || state.calibrating.test_and_set(cat::memory_order::acquire)
   ) {
      return;
   }

   conversion const old = read_conversion();
   if (read_tsc() < state.next_calibration.relaxed().load()) {
      state.calibrating.clear(cat::memory_order::release);
      return;
   }

   cat::maybe<synchronized_time> const synchronized = synchronize_time();
   if (synchronized.is_empty() || synchronized.value().tsc <= old.base_tsc) {
      state.calibrating.clear(cat::memory_order::release);
      return;
   }

   cat::maybe<cat::int8> const predicted =
      to_nanoseconds(synchronized.value().tsc, old);
   if (predicted.is_empty()) {
      state.calibrating.clear(cat::memory_order::release);
      return;
   }

   cat::int8 error = predicted.value() - synchronized.value().nanoseconds;
   if (error > maximum_error) {
      error = maximum_error;
   } else if (error < -maximum_error) {
      error = -maximum_error;
   }

   cat::int8 const predicted_elapsed = predicted.value() - old.base_nanoseconds;
   cat::int8 const base_error = state.base_error.relaxed();
   cat::int8 const corrected_elapsed =
      predicted_elapsed - (error * 2 - base_error);
   cat::maybe<cat::uint8> const multiplier = make_multiplier(
      corrected_elapsed, synchronized.value().tsc - old.base_tsc
   );
   if (multiplier.has_value()) {
      write_conversion(
         {
            .base_tsc = synchronized.value().tsc,
            .base_nanoseconds = predicted.value(),
            .multiplier = multiplier.value(),
         },
         error, next_calibration(synchronized.value().tsc, multiplier.value())
      );
   }

   state.calibrating.clear(cat::memory_order::release);
}
