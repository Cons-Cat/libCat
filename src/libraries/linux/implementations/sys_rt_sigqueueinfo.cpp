#include <cat/linux>

auto
nix::sys_rt_sigqueueinfo(process_id pid, signal s, signal_info const& info)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(129, pid, s, &info);
}
