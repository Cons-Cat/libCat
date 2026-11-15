#include <cat/linux>

auto
nix::sys_sched_setparam(
   process_id pid, scheduling_parameters const& parameters
) noexcept -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(142, pid, &parameters);
}
