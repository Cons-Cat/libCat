#include <cat/linux>

auto
nix::sys_sched_setscheduler(
   process_id pid, scheduling_policy policy,
   scheduling_parameters const& parameters
) noexcept -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(144, pid, policy, &parameters);
}
