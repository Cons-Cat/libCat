#include <cat/linux>

auto
nix::sys_sched_rr_get_interval(process_id pid, timespec& out) noexcept
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(148, pid, &out);
}
