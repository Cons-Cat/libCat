#include <cat/detail/vdso.hpp>

#include <cat/chrono>
#include <cat/linux>
#include <cat/thread>

#include "../unit_tests.hpp"

namespace {

using attoseconds = cat::units::attosecond_int8;
using femtoseconds = cat::units::femtosecond_int8;
using picoseconds = cat::units::picosecond_int8;
using nanoseconds = cat::units::nanosecond_int8;
using milliseconds = cat::units::millisecond_int8;
using seconds = cat::units::second_int8;
using steady_time = cat::clock_steady::time_point;
using steady_millisecond_time =
   cat::time_point<cat::clock_steady, milliseconds>;
using scaled_clock = cat::clock_scaled<cat::clock_steady>;

using monotonic_timer = nix::clock_timerfd<nix::clock_id::monotonic>;
using realtime_timer = nix::clock_timerfd<nix::clock_id::realtime>;

template <typename Point>
concept can_sleep_until =
   requires(Point point) { cat::this_thread::sleep_until(point); };

auto
clock_gettime_ok(nix::clock_id clock) -> bool {
   nix::timespec value;
   nix::scaredy_nix<void> const result = nix::sys_clock_gettime(clock, value);
   if (result.has_value()) {
      cat::verify(
         value.nanoseconds >= nanoseconds(0)
         && value.nanoseconds < nanoseconds(1'000'000'000)
      );
      return true;
   }
   return result.error() == nix::linux_error::perm
          || result.error() == nix::linux_error::acces
          || result.error() == nix::linux_error::inval
          || result.error() == nix::linux_error::nosys;
}

}  // namespace

$test(timespec_conversions) {
   static_assert(sizeof(nix::timespec) == 16);
   static_assert(sizeof(nix::itimerspec) == 32);
   static_assert(__is_same(nix::futex_timespec, nix::timespec));

   nix::timespec const zero = nix::make_timespec(nanoseconds(0));
   cat::verify(zero.seconds == seconds(0));
   cat::verify(zero.nanoseconds == nanoseconds(0));
   cat::verify(zero.to_nanoseconds() == nanoseconds(0));

   nix::timespec const one_second =
      nix::make_timespec(nanoseconds(1'000'000'000));
   cat::verify(one_second.seconds == seconds(1));
   cat::verify(one_second.nanoseconds == nanoseconds(0));

   nix::timespec const from_ns = nix::make_timespec(nanoseconds(1'500'000'000));
   cat::verify(from_ns.seconds == seconds(1));
   cat::verify(from_ns.nanoseconds == nanoseconds(500'000'000));
   cat::verify(from_ns.to_nanoseconds() == nanoseconds(1'500'000'000));

   nix::timespec const from_ms = nix::make_timespec(milliseconds(2));
   cat::verify(from_ms.seconds == seconds(0));
   cat::verify(from_ms.nanoseconds == nanoseconds(2'000'000));
   cat::verify(from_ms.to_nanoseconds() == nanoseconds(2'000'000));

   nix::timespec const negative =
      nix::make_timespec(nanoseconds(-1'500'000'000));
   cat::verify(negative.seconds == seconds(-2));
   cat::verify(negative.nanoseconds == nanoseconds(500'000'000));
   cat::verify(negative.to_nanoseconds() == nanoseconds(-1'500'000'000));
}

$test(clock_syscalls) {
   cat::verify(clock_gettime_ok(nix::clock_id::realtime));
   cat::verify(clock_gettime_ok(nix::clock_id::monotonic));
   cat::verify(clock_gettime_ok(nix::clock_id::monotonic_raw));
   cat::verify(clock_gettime_ok(nix::clock_id::monotonic_coarse));
   cat::verify(clock_gettime_ok(nix::clock_id::realtime_coarse));
   cat::verify(clock_gettime_ok(nix::clock_id::boottime));
   cat::verify(clock_gettime_ok(nix::clock_id::process_cpu_time));
   cat::verify(clock_gettime_ok(nix::clock_id::thread_cpu_time));
   cat::verify(clock_gettime_ok(nix::clock_id::tai));
   cat::verify(clock_gettime_ok(nix::clock_id::realtime_alarm));
   cat::verify(clock_gettime_ok(nix::clock_id::boottime_alarm));

   nix::timespec realtime;
   nix::sys_clock_gettime(nix::clock_id::realtime, realtime).verify();
   cat::verify(realtime.seconds >= seconds(0));

   nix::timespec resolution;
   nix::sys_clock_getres(nix::clock_id::monotonic, resolution).verify();
   cat::verify(resolution.to_nanoseconds() > nanoseconds(0));
   nix::sys_clock_getres(nix::clock_id::realtime, resolution).verify();
   cat::verify(resolution.to_nanoseconds() > nanoseconds(0));

   nix::timespec remaining{};
   nix::timespec const zero = nix::make_timespec(nanoseconds(0));
   nix::sys_nanosleep(zero, &remaining).verify();
   nix::sys_nanosleep(zero).verify();

   nix::sys_clock_nanosleep(
      nix::clock_id::monotonic, nix::clock_nanosleep_flags::none, zero,
      &remaining
   )
      .verify();

   nix::timespec now;
   nix::sys_clock_gettime(nix::clock_id::monotonic, now).verify();
   nix::timespec const soon =
      nix::make_timespec(now.to_nanoseconds() + nanoseconds(1'000'000));
   nix::sys_clock_nanosleep(
      nix::clock_id::monotonic, nix::clock_nanosleep_flags::absolute, soon
   )
      .verify();
}

#ifndef CAT_NO_VDSO
$test(vdso_time_hooks) {
   nix::timespec now;
   cat::maybe<cat::int4> const result =
      nix::detail::vdso_clock_gettime(nix::clock_id::monotonic, now);
   cat::verify(result.has_value());
   cat::verify(result.value() == 0);
   cat::verify(now.to_nanoseconds() > nanoseconds(0));

   nix::timespec resolution;
   cat::maybe<cat::int4> const resolution_result =
      nix::detail::vdso_clock_getres(nix::clock_id::monotonic, resolution);
   if (resolution_result.has_value()) {
      cat::verify(resolution_result.value() == 0);
      cat::verify(resolution.to_nanoseconds() > nanoseconds(0));
   }

   nix::detail::timeval wall_time;
   cat::maybe<cat::int4> const gettimeofday_result =
      nix::detail::vdso_gettimeofday(&wall_time, nullptr);
   if (gettimeofday_result.has_value()) {
      cat::verify(gettimeofday_result.value() == 0);
      cat::verify(wall_time.seconds > 0);
   }

   cat::maybe<cat::int8> const time_result = nix::detail::vdso_time(nullptr);
   if (time_result.has_value()) {
      cat::verify(time_result.value() > 0);
   }
}
#endif

$test(timerfd_syscalls) {
   nix::file_descriptor const fd =
      nix::sys_timerfd_create(
         nix::clock_id::monotonic, nix::timerfd_flags::close_exec
      )
         .verify();

   nix::itimerspec const one_shot = {
      .interval = nix::make_timespec(nanoseconds(0)),
      .value = nix::make_timespec(milliseconds(1)),
   };
   nix::sys_timerfd_settime(fd, nix::timerfd_set_flags::none, one_shot)
      .verify();

   nix::itimerspec armed;
   nix::sys_timerfd_gettime(fd, armed).verify();
   cat::verify(armed.interval.to_nanoseconds() == nanoseconds(0));
   cat::verify(armed.value.to_nanoseconds() > nanoseconds(0));

   nix::itimerspec old{};
   nix::itimerspec const disarmed = {
      .interval = nix::make_timespec(nanoseconds(0)),
      .value = nix::make_timespec(nanoseconds(0)),
   };
   nix::sys_timerfd_settime(fd, nix::timerfd_set_flags::none, disarmed, &old)
      .verify();
   cat::verify(old.value.to_nanoseconds() > nanoseconds(0));
   nix::sys_close(fd).verify();
}

$test(os_timer) {
   monotonic_timer timer =
      nix::create_clock_timerfd(nix::timerfd_flags::close_exec).verify();
   cat::verify(timer.descriptor().value > 2u);

   nix::itimerspec const spec = {
      .interval = nix::make_timespec(milliseconds(1)),
      .value = nix::make_timespec(milliseconds(1)),
   };
   timer.set(spec).verify();

   nix::itimerspec remaining;
   timer.get(remaining).verify();
   cat::verify(remaining.interval.to_nanoseconds() == nanoseconds(1'000'000));
   cat::verify(remaining.value.to_nanoseconds() > nanoseconds(0));

   cat::verify(timer.wait().verify() >= 1u);
   cat::verify(timer.wait().verify() >= 1u);

   nix::itimerspec old{};
   nix::itimerspec const disarmed = {
      .interval = nix::make_timespec(nanoseconds(0)),
      .value = nix::make_timespec(nanoseconds(0)),
   };
   timer.set(disarmed, nix::timerfd_set_flags::none, &old).verify();
   timer.set(milliseconds(1)).verify();
   cat::verify(timer.wait().verify() >= 1u);

   nix::timespec now;
   nix::sys_clock_gettime(nix::clock_id::monotonic, now).verify();
   nix::itimerspec const absolute = {
      .interval = nix::make_timespec(nanoseconds(0)),
      .value =
         nix::make_timespec(now.to_nanoseconds() + nanoseconds(1'000'000)),
   };
   timer.set(absolute, nix::timerfd_set_flags::absolute).verify();
   cat::verify(timer.wait().verify() >= 1u);
   cat::verify(
      monotonic_timer::now().verify().time_since_epoch() > nanoseconds(0)
   );
   timer.close().verify();

   monotonic_timer idle =
      nix::create_clock_timerfd(
         nix::timerfd_flags::close_exec | nix::timerfd_flags::nonblocking
      )
         .verify();
   nix::scaredy_nix<cat::uint8> const idle_wait = idle.wait();
   cat::verify(idle_wait.is_empty());
   cat::verify(idle_wait.error() == nix::linux_error::again);
   idle.close().verify();

   nix::scaredy_nix<nix::clock_timerfd<nix::clock_id::realtime_alarm>> alarm =
      nix::create_clock_timerfd<nix::clock_id::realtime_alarm>();
   if (alarm.has_value()) {
      alarm.value().close().verify();
   } else {
      cat::verify(
         alarm.error() == nix::linux_error::perm
         || alarm.error() == nix::linux_error::acces
         || alarm.error() == nix::linux_error::inval
      );
   }
}

$test(chrono_durations) {
   static_assert(cat::is_duration<attoseconds>);
   static_assert(cat::is_duration<femtoseconds>);
   static_assert(cat::is_duration<picoseconds>);
   static_assert(cat::is_duration<cat::units::attosecond_float8>);
   static_assert(cat::is_duration<cat::units::femtosecond_float8>);
   static_assert(cat::is_duration<cat::units::picosecond_float8>);
   static_assert(__is_same(
      cat::units::attoseconds, cat::reference_quantity<cat::units::attosecond>
   ));
   static_assert(__is_same(
      cat::units::femtoseconds, cat::reference_quantity<cat::units::femtosecond>
   ));
   static_assert(__is_same(
      cat::units::picoseconds, cat::reference_quantity<cat::units::picosecond>
   ));
   static_assert(
      __is_same(attoseconds::quantity_type, cat::units::attoseconds)
   );
   static_assert(
      __is_same(femtoseconds::quantity_type, cat::units::femtoseconds)
   );
   static_assert(
      __is_same(picoseconds::quantity_type, cat::units::picoseconds)
   );

   // These conversions all stay well inside of an `int8`.
   static_assert(attoseconds(picoseconds(1)) == attoseconds(1'000'000));
   static_assert(attoseconds(femtoseconds(1)) == attoseconds(1'000));
   static_assert(femtoseconds(picoseconds(1)) == femtoseconds(1'000));
   static_assert(picoseconds(nanoseconds(1)) == picoseconds(1'000));
   static_assert(attoseconds(nanoseconds(1)) == attoseconds(1'000'000'000));
   static_assert(picoseconds(attoseconds(2'000'000)) == picoseconds(2));
   static_assert(picoseconds(femtoseconds(3'000)) == picoseconds(3));
   static_assert(nanoseconds(picoseconds(3'000)) == nanoseconds(3));
   static_assert(picoseconds(1) + picoseconds(2) == picoseconds(3));
   static_assert(attoseconds(5).raw == 5);
   static_assert(femtoseconds(5).raw == 5);
   static_assert(picoseconds(5).raw == 5);
}

$test(chrono_time_point) {
   static_assert(cat::is_duration<nanoseconds>);
   static_assert(cat::is_duration<milliseconds>);
   static_assert(cat::is_duration<seconds>);
   static_assert(cat::is_duration<cat::units::minute_int4>);
   static_assert(!cat::is_duration<cat::units::metre_int8>);
   static_assert(!cat::is_duration<cat::units::hertz_int8>);
   static_assert(!cat::is_duration<cat::int8>);

   // A `time_point` is one tick, not a `{seconds, nanoseconds}` pair, so
   // arithmetic on it is one integer operation.
   static_assert(sizeof(steady_time) == 8);
   static_assert(sizeof(steady_millisecond_time) == 8);
   static_assert(sizeof(cat::unix_time_point) == 8);

   constexpr steady_time epoch{};
   constexpr steady_time five{nanoseconds(5)};
   static_assert(epoch.time_since_epoch() == nanoseconds(0));
   static_assert(five.time_since_epoch() == nanoseconds(5));
   static_assert((five + nanoseconds(3)).time_since_epoch() == nanoseconds(8));
   static_assert((nanoseconds(3) + five).time_since_epoch() == nanoseconds(8));
   static_assert((five - nanoseconds(2)).time_since_epoch() == nanoseconds(3));
   static_assert(five - steady_time{nanoseconds(2)} == nanoseconds(3));
   static_assert(five > epoch);
   static_assert(five == five);
   static_assert(epoch < five);
   static_assert(nanoseconds(milliseconds(2)) == nanoseconds(2'000'000));
   static_assert(milliseconds(nanoseconds(2'000'000)) == milliseconds(2));

   // A coarser `time_point` of the same clock widens implicitly, but the
   // lossy direction stays explicit.
   constexpr steady_millisecond_time two_milliseconds{milliseconds(2)};
   constexpr steady_time widened = two_milliseconds;
   static_assert(widened.time_since_epoch() == nanoseconds(2'000'000));
   static_assert(cat::is_convertible<steady_millisecond_time, steady_time>);
   static_assert(!cat::is_convertible<steady_time, steady_millisecond_time>);

   static_assert(steady_time::min().time_since_epoch() == nanoseconds::min());
   static_assert(steady_time::max().time_since_epoch() == nanoseconds::max());
   static_assert(steady_time::min() < steady_time::max());
   static_assert(
      steady_millisecond_time::max().time_since_epoch() == milliseconds::max()
   );

   steady_time mutating{nanoseconds(5)};
   mutating += nanoseconds(4);
   cat::verify(mutating.time_since_epoch() == nanoseconds(9));
   mutating -= nanoseconds(1);
   cat::verify(mutating.time_since_epoch() == nanoseconds(8));

   // Stepping moves by one tick of the point's own duration.
   steady_time stepping{nanoseconds(5)};
   cat::verify((++stepping).time_since_epoch() == nanoseconds(6));
   cat::verify((stepping++).time_since_epoch() == nanoseconds(6));
   cat::verify(stepping.time_since_epoch() == nanoseconds(7));
   cat::verify((--stepping).time_since_epoch() == nanoseconds(6));
   cat::verify((stepping--).time_since_epoch() == nanoseconds(6));
   cat::verify(stepping.time_since_epoch() == nanoseconds(5));

   cat::unix_time_point second_tick{seconds(10)};
   cat::verify((++second_tick).time_since_epoch() == seconds(11));
   cat::verify((--second_tick).time_since_epoch() == seconds(10));
}

$test(chrono_clocks) {
   static_assert(cat::is_clock<cat::clock_system>);
   static_assert(cat::is_clock<cat::clock_steady>);
   static_assert(cat::is_clock<cat::clock_unix>);
   static_assert(cat::is_clock<cat::clock_utc>);
   static_assert(cat::is_clock<cat::clock_tai>);
   static_assert(cat::is_clock<cat::clock_gps>);
   static_assert(cat::is_clock<scaled_clock>);
   static_assert(cat::is_clock<monotonic_timer>);
   static_assert(cat::is_clock<realtime_timer>);
   static_assert(!cat::is_clock<cat::local_t>);
   static_assert(!cat::is_clock<nanoseconds>);
   static_assert(!cat::is_clock<steady_time>);

   static_assert(!cat::clock_system::is_steady);
   static_assert(cat::clock_steady::is_steady);
   static_assert(!scaled_clock::is_steady);
   static_assert(monotonic_timer::is_steady);
   static_assert(!realtime_timer::is_steady);
   static_assert(
      __is_same(cat::clock_system::duration, cat::units::nanosecond_int8)
   );
   static_assert(__is_same(nix::clock_timerfd<>, monotonic_timer));
   static_assert(monotonic_timer::id == nix::clock_id::monotonic);
   static_assert(realtime_timer::id == nix::clock_id::realtime);

   // Each kernel clock has its own `time_point`, so a deadline of one cannot
   // be mistaken for a deadline of another.
   static_assert(
      !__is_same(monotonic_timer::time_point, realtime_timer::time_point)
   );

   // A wrapping clock names its kernel clock through `native_handle()`.
   static_assert(__is_same(
      __typeof_unqual(cat::clock_steady::native_handle()), monotonic_timer
   ));
   static_assert(__is_same(
      __typeof_unqual(cat::clock_system::native_handle()), realtime_timer
   ));
   static_assert(__is_same(
      __typeof_unqual(cat::clock_unix::native_handle()), realtime_timer
   ));
   static_assert(can_sleep_until<steady_time>);
   static_assert(can_sleep_until<monotonic_timer::time_point>);
   static_assert(can_sleep_until<cat::clock_tai::time_point>);
   static_assert(can_sleep_until<cat::clock_utc::time_point>);
   static_assert(can_sleep_until<cat::clock_gps::time_point>);
   static_assert(can_sleep_until<scaled_clock::time_point>);
   static_assert(!can_sleep_until<cat::local_time_point>);

   static_assert(
      __is_same(decltype(cat::clock_steady::now()), cat::maybe<steady_time>)
   );
   static_assert(__is_same(
      decltype(cat::this_thread::sleep_for(milliseconds(1))), cat::maybe<void>
   ));

   cat::clock_steady::time_point const start =
      cat::clock_steady::now().verify();
   cat::this_thread::sleep_for(milliseconds(1)).verify();
   cat::clock_steady::time_point const after_sleep =
      cat::clock_steady::now().verify();
   cat::verify(after_sleep > start);
   cat::verify(after_sleep - start >= milliseconds(1));

   cat::clock_system::time_point const system_now =
      cat::clock_system::now().verify();
   cat::verify(system_now.time_since_epoch() > nanoseconds(0));

   cat::clock_steady::time_point const soon =
      cat::clock_steady::now().verify() + milliseconds(1);
   cat::this_thread::sleep_until(soon).verify();
   cat::verify(cat::clock_steady::now().verify() >= soon);
}

$test(chrono_epochs) {
   // GPS time began at 1980-01-06, 315'964'809 atomic seconds after the UTC
   // epoch of 1970-01-01.
   static_assert(
      cat::to_gps(cat::utc_time_point(315'964'809)).time_since_epoch()
      == seconds(0)
   );
   static_assert(
      cat::to_utc(cat::gps_time_point(0)).time_since_epoch()
      == seconds(315'964'809)
   );
   static_assert(
      cat::to_tai(cat::gps_time_point(0)).time_since_epoch()
      == seconds(694'656'019)
   );
   static_assert(
      cat::to_gps(cat::tai_time_point(694'656'019)).time_since_epoch()
      == seconds(0)
   );
   static_assert(
      cat::to_gps(cat::to_tai(cat::gps_time_point(1'000)))
      == cat::gps_time_point(1'000)
   );
   static_assert(
      cat::to_utc(cat::to_gps(cat::utc_time_point(1'000'000'000)))
      == cat::utc_time_point(1'000'000'000)
   );

   cat::clock_tai::time_point const tai = cat::clock_tai::now().verify();
   cat::clock_utc::time_point const utc = cat::clock_utc::now().verify();
   cat::clock_gps::time_point const gps = cat::clock_gps::now().verify();
   cat::clock_unix::time_point const posix = cat::clock_unix::now().verify();

   // Every epoch is read from one kernel clock, so these gaps are fixed. The
   // tolerance only absorbs the delay between reads.
   constexpr nanoseconds tolerance = nanoseconds(seconds(2));
   constexpr nanoseconds tai_utc_gap = nanoseconds(seconds(378'691'210));
   constexpr nanoseconds utc_gps_gap = nanoseconds(seconds(315'964'809));
   nanoseconds const tai_utc = tai.time_since_epoch() - utc.time_since_epoch();
   nanoseconds const utc_gps = utc.time_since_epoch() - gps.time_since_epoch();
   cat::verify(tai_utc >= tai_utc_gap - tolerance);
   cat::verify(tai_utc <= tai_utc_gap + tolerance);
   cat::verify(utc_gps >= utc_gps_gap - tolerance);
   cat::verify(utc_gps <= utc_gps_gap + tolerance);
   cat::verify(posix.time_since_epoch() > nanoseconds(0));
}

$test(chrono_scaled_clock) {
   cat::clock_steady::time_point const before =
      cat::clock_steady::now().verify();
   scaled_clock::time_point const unscaled = scaled_clock::now().verify();
   cat::clock_steady::time_point const after =
      cat::clock_steady::now().verify();

   // A speed of 1 tracks the steady clock it wraps.
   cat::verify(
      unscaled.time_since_epoch() >= before.time_since_epoch() - milliseconds(1)
   );
   cat::verify(
      unscaled.time_since_epoch() <= after.time_since_epoch() + milliseconds(1)
   );

   scaled_clock::set_speed(2.0f);
   cat::clock_steady::time_point const steady =
      cat::clock_steady::now().verify();
   scaled_clock::time_point const doubled = scaled_clock::now().verify();
   nanoseconds const scaled_elapsed = doubled.time_since_epoch();
   nanoseconds const steady_elapsed = steady.time_since_epoch();
   cat::verify(scaled_elapsed >= steady_elapsed);
   cat::verify(
      scaled_elapsed - steady_elapsed >= steady_elapsed - milliseconds(1)
   );

   scaled_clock::set_speed(cat::float4(1.0f));
   scaled_clock::time_point const restored = scaled_clock::now().verify();
   cat::verify(
      restored.time_since_epoch()
      <= cat::clock_steady::now().verify().time_since_epoch() + milliseconds(1)
   );
}

$test(chrono_sleep_until) {
   static_assert(__is_same(
      decltype(cat::this_thread::sleep_until(scaled_clock::time_point{})),
      cat::maybe<void>
   ));

   // A scaled clock names no kernel clock, so it sleeps relative to its own
   // `now()`. A deadline which has already passed returns without sleeping.
   scaled_clock::time_point const passed = scaled_clock::now().verify();
   cat::this_thread::sleep_until(passed).verify();
   cat::verify(scaled_clock::now().verify() >= passed);

   cat::clock_tai::time_point const tai = cat::clock_tai::now().verify();
   cat::this_thread::sleep_until(tai - milliseconds(1)).verify();
   cat::clock_gps::time_point const gps = cat::clock_gps::now().verify();
   cat::this_thread::sleep_until(gps - milliseconds(1)).verify();

   // A kernel clock is slept against absolutely, so a deadline in the past
   // returns at once and a deadline ahead of it is reached.
   cat::this_thread::sleep_until(monotonic_timer::time_point(nanoseconds(0)))
      .verify();
   monotonic_timer::time_point const soon =
      monotonic_timer::now().verify() + milliseconds(1);
   cat::this_thread::sleep_until(soon).verify();
   cat::verify(monotonic_timer::now().verify() >= soon);

   // A clock which wraps a kernel clock is slept against that clock.
   cat::clock_unix::time_point const unix_soon =
      cat::clock_unix::now().verify() + milliseconds(1);
   cat::this_thread::sleep_until(unix_soon).verify();
   cat::verify(cat::clock_unix::now().verify() >= unix_soon);

   cat::clock_system::time_point const system_soon =
      cat::clock_system::now().verify() + milliseconds(1);
   cat::this_thread::sleep_until(system_soon).verify();
   cat::verify(cat::clock_system::now().verify() >= system_soon);
}
