#include <cat/detail/vdso.hpp>

namespace {

using gettimeofday_hook =
   int (*)(nix::detail::timeval* _Nullable, nix::detail::timezone* _Nullable);
using time_hook = cat::int8 (*)(cat::int8* _Nullable);

gettimeofday_hook p_gettimeofday = nullptr;
time_hook p_time = nullptr;
bool gettimeofday_resolved = false;
bool time_resolved = false;

}  // namespace

auto
nix::detail::vdso_gettimeofday(
   timeval* _Nullable p_time, timezone* _Nullable p_timezone
) -> cat::maybe<cat::int4> {
   gettimeofday_hook const p_hook = resolve_vdso_hook(
      p_gettimeofday, gettimeofday_resolved, "__vdso_gettimeofday",
      "gettimeofday"
   );
   return call_vdso_hook(p_hook, p_time, p_timezone);
}

auto
nix::detail::vdso_time(cat::int8* _Nullable p_time) -> cat::maybe<cat::int8> {
   time_hook const p_hook =
      resolve_vdso_hook(::p_time, time_resolved, "__vdso_time", "time");
   return call_vdso_hook(p_hook, p_time);
}
