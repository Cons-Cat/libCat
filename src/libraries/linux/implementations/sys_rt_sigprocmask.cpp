#include <cat/linux>

auto nix::sys_rt_sigprocmask(SignalAction action,
                             SignalsMaskSet const* __restrict p_other_set,
                             SignalsMaskSet* __restrict p_current_set)
    -> ScaredyLinux<void> {
    // TODO: 8 was `_NSIG / 8`. Explain this.
    ScaredyLinux<void> result =
        syscall<void>(14, action, p_other_set, p_current_set, 8);
    return result;
}
