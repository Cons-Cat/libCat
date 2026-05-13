#include <cat/linux>

auto
nix::sys_rt_sigsuspend(signals_mask_set const& new_mask)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(130, &new_mask, sizeof(signals_mask_set));
}
