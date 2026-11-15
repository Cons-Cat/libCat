#include <cat/linux>

auto
nix::sys_sched_get_priority_max(scheduling_policy policy) noexcept
   -> nix::scaredy_nix<cat::int4> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<cat::int4>(146, policy);
}
