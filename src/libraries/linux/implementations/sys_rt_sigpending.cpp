#include <cat/linux>

auto
nix::sys_rt_sigpending(signals_mask_set& out) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(127, &out, sizeof(signals_mask_set));
}
