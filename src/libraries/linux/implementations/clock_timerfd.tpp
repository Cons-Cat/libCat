// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/debug>
#include <cat/linux>
#include <cat/maybe>
#include <cat/propagate>

consteval auto
nix::is_steady_clock_id(clock_id clock) -> bool {
   return clock == clock_id::monotonic || clock == clock_id::monotonic_raw
          || clock == clock_id::monotonic_coarse || clock == clock_id::boottime
          || clock == clock_id::boottime_alarm
          || clock == clock_id::process_cpu_time
          || clock == clock_id::thread_cpu_time;
}

template <nix::clock_id clock>
auto
nix::create_clock_timerfd(timerfd_flags flags)
   -> nix::scaredy_nix<nix::clock_timerfd<clock>> {
   return clock_timerfd<clock>($prop(sys_timerfd_create(clock, flags)));
}

template <nix::clock_id clock>
auto
nix::clock_timerfd<clock>::now() -> cat::maybe<time_point> {
   timespec value;
   $prop_as(sys_clock_gettime(id, value), cat::nullopt);
   return time_point(value.to_nanoseconds());
}

template <nix::clock_id clock>
auto
nix::clock_timerfd<clock>::set(
   itimerspec const& value, timerfd_set_flags flags, itimerspec* _Nullable p_old
) -> nix::scaredy_nix<void> {
   return sys_timerfd_settime(m_descriptor, flags, value, p_old);
}

template <nix::clock_id clock>
auto
nix::clock_timerfd<clock>::get(itimerspec& out) const
   -> nix::scaredy_nix<void> {
   return sys_timerfd_gettime(m_descriptor, out);
}

template <nix::clock_id clock>
auto
nix::clock_timerfd<clock>::wait() const -> nix::scaredy_nix<cat::uint8> {
   cat::uint8 expirations;
   cat::idx const read = $prop(sys_read(
      m_descriptor, __builtin_bit_cast(char* _Nonnull, &expirations),
      sizeof(expirations)
   ));
   cat::verify(read == sizeof(expirations));
   return expirations;
}

template <nix::clock_id clock>
auto
nix::clock_timerfd<clock>::close() -> nix::scaredy_nix<void> {
   return sys_close(m_descriptor);
}

// Optimize `cat::this_thread::sleep_until` for `nix::clock_timerfd`.
template <nix::clock_id clock, cat::is_duration Duration>
auto
cat::this_thread::sleep_until(
   time_point<nix::clock_timerfd<clock>, Duration> const& point
) -> maybe<void> {
   nix::timespec const request = nix::make_timespec(point.time_since_epoch());
   while (true) {
      scaredy result = nix::sys_clock_nanosleep(
         clock, nix::clock_nanosleep_flags::absolute, request
      );

      if (result.has_value()) {
         return monostate;
      }

      // Retry the loop if we get an interrupt.
      if (result.error() != nix::linux_error::intr) {
         return nullopt;
      }
   }
}
