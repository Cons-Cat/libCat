#include <cat/linux>

auto
nix::sys_sched_setattr(
   process_id pid, scheduling_attributes const& attributes
) noexcept -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   // The trailing `flags` argument is reserved and must be 0.
   return nix::syscall_volatile<void>(314, pid, &attributes, 0u);
}
