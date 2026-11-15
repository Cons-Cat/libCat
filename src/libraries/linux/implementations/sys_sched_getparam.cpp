#include <cat/linux>

auto
nix::sys_sched_getparam(process_id pid, scheduling_parameters& out) noexcept
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(143, pid, &out);
}
