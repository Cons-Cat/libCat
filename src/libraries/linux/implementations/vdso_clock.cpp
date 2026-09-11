#include <cat/detail/vdso.hpp>

namespace {

using clock_hook = cat::int4 (*_Nonnull)(int, nix::timespec* _Nonnull);

clock_hook p_clock_gettime = nullptr;
clock_hook p_clock_getres = nullptr;
bool clock_gettime_resolved = false;
bool clock_getres_resolved = false;

}  // namespace

auto
nix::detail::vdso_clock_gettime(clock_id clock, timespec& out)
   -> cat::maybe<cat::int4> {
   clock_hook const p_hook = resolve_vdso_hook(
      p_clock_gettime, clock_gettime_resolved, "__vdso_clock_gettime",
      "clock_gettime"
   );
   return call_vdso_hook(p_hook, static_cast<int>(clock), &out);
}

auto
nix::detail::vdso_clock_getres(clock_id clock, timespec& out)
   -> cat::maybe<cat::int4> {
   clock_hook const p_hook = resolve_vdso_hook(
      p_clock_getres, clock_getres_resolved, "__vdso_clock_getres",
      "clock_getres"
   );
   return call_vdso_hook(p_hook, static_cast<int>(clock), &out);
}
