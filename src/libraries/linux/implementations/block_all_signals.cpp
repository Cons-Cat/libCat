#include <cat/linux>

auto nix::block_all_signals() -> SignalsMaskSet {
    SignalsMaskSet current_mask;
    sys_rt_sigprocmask(SignalAction::block, &all_signals_mask, &current_mask);
    return current_mask;
}
