#include <cat/linux>

auto
nix::sys_rt_sigtimedwait(signals_mask_set const& set, signal_info* p_info,
                         futex_timespec const* p_timeout)
   -> nix::scaredy_nix<signal> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<signal>(128, &set, p_info, p_timeout,
                                        sizeof(signals_mask_set));
}
