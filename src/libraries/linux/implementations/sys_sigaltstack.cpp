#include <cat/linux>

auto
nix::sys_sigaltstack(signal_stack const* _Nullable __restrict p_new_stack,
                     signal_stack* _Nullable __restrict p_old_stack)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(131, p_new_stack, p_old_stack);
}
