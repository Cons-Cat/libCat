#include <cat/linux>

auto nix::sys_rt_sigprocmask(SignalAction action,
                             SignalsMaskSet const* __restrict p_other_set,
                             SignalsMaskSet* __restrict p_current_set)
    -> ScaredyLinux<void> {
    ScaredyLinux<void> result = syscall<void>(
        14, action, p_other_set, p_current_set,
        // Except for MIPS, there are 64 signals in the Linux kernel.
        (64 + 1) / 8);
    return result;
}
