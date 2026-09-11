#include <cat/linux>

auto
nix::sys_nanosleep(timespec const& request, timespec* _Nullable p_remaining)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(35, &request, p_remaining);
}
