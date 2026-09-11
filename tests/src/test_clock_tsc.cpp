#include <cat/chrono>

#include "../unit_tests.hpp"

$test(clock_tsc) {
   static_assert(cat::is_clock<x64::clock_tsc>);
   static_assert(x64::clock_tsc::is_steady);
   static_assert(sizeof(x64::clock_tsc::time_point) == 8);

   cat::maybe<x64::clock_tsc::time_point> const first = x64::clock_tsc::now();
   if (first.is_empty()) {
      return;
   }

   cat::maybe<x64::clock_tsc::time_point> const second = x64::clock_tsc::now();
   cat::verify(second.has_value());
   cat::verify(second.value() >= first.value());

   x64::clock_tsc::calibrate();
   cat::maybe<x64::clock_tsc::time_point> const calibrated =
      x64::clock_tsc::now();
   cat::verify(calibrated.has_value());
   cat::verify(calibrated.value() >= second.value());
}
