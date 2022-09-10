#include <cat/linux>

auto nix::raise(Signal signal, ProcessId pid) -> ScaredyLinux<void> {
    SignalsMaskSet current_mask = block_all_signals();
    cat::Scaredy result = sys_tkill(pid, signal);
    sys_rt_sigprocmask(SignalAction::set_mask, &current_mask, nullptr);
    return result;
}
