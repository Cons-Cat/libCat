#include <cat/linux>

auto
nix::sys_sched_getattr(process_id pid, scheduling_attributes& out) noexcept
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   // The trailing `flags` argument is reserved and must be 0.
   return nix::syscall_volatile<void>(
      315, pid, &out, cat::uint4(sizeof(scheduling_attributes)), 0u
   );
}
