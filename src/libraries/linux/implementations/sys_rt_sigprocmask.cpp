#include <cat/linux>

auto nix::sys_rt_sigprocmask(signal_action action,
                             signals_mask_set const* __restrict p_other_set,
                             signals_mask_set* __restrict p_current_set)
    -> scaredy_nix<void> {
    scaredy_nix<void> result =
        syscall4(14, action, p_other_set, p_current_set,
                 // Except for MIPS, there are 64 signals in the Linux kernel.
                 (64 + 1) / 8);
    return result;
}
