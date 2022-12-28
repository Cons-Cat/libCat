#include <cat/linux>

auto nix::raise(Signal signal, process_id pid) -> scaredy_nix<void> {
    signals_mask_set current_mask = block_all_signals();
    cat::scaredy result = sys_tkill(pid, signal);
    sys_rt_sigprocmask(signal_action::set_mask, &current_mask, nullptr);
    return result;
}
