// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/linux>
#include <cat/metric_units>

// Add `seconds` and `nanoseconds` as a quantity of
// `cat::units::nanoseconds`.
constexpr auto
nix::timespec::to_nanoseconds() const -> cat::units::nanosecond_int8 {
   return cat::units::nanosecond_int8(seconds) + nanoseconds;
}

template <cat::is_duration Duration>
constexpr auto
nix::make_timespec(Duration elapsed) -> nix::timespec {
   cat::units::nanosecond_int8 const ns(elapsed);
   cat::int8 const total = ns.raw;
   cat::int8 const billion = 1'000'000'000;
   cat::int8 seconds = total / billion;
   cat::int8 nanoseconds = total % billion;
   if (nanoseconds < 0) {
      seconds--;
      nanoseconds += billion;
   }
   return {
      .seconds = seconds * cat::units::second,
      .nanoseconds = nanoseconds * cat::units::nanosecond,
   };
}
