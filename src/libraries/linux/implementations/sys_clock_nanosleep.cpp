#include <cat/linux>

auto
nix::sys_clock_nanosleep(
   clock_id clock, clock_nanosleep_flags flags, timespec const& request,
   timespec* _Nullable p_remaining
) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(230, clock, flags, &request, p_remaining);
}
