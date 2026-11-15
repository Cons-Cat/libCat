#include <cat/linux>

auto
nix::sys_sched_getscheduler(process_id pid) noexcept
   -> nix::scaredy_nix<scheduling_policy> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<scheduling_policy>(145, pid);
}
