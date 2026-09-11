#include <cat/linux>

auto
nix::sys_timerfd_settime(
   file_descriptor file_descriptor, timerfd_set_flags flags,
   itimerspec const& value, itimerspec* _Nullable p_old
) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(
      286, file_descriptor, flags, &value, p_old
   );
}
