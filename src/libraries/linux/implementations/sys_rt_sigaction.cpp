#include <cat/linux>

auto
nix::sys_rt_sigaction(
   signal s, sigaction const* _Nullable __restrict p_act,
   sigaction* _Nullable __restrict p_old_act
) -> nix::scaredy_nix<void> {
   // The kernel ABI takes `sigsetsize` as the 4th argument and rejects any
   // value other than `sizeof(sigset_t)`. On x86-64 that is 8 bytes.
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(
      13, s, p_act, p_old_act, sizeof(signals_mask_set)
   );
}
