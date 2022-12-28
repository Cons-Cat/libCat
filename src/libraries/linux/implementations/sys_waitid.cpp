#include <cat/linux>

auto nix::sys_waitid(wait_id type, process_id pid, wait_options_flags options)
    -> scaredy_nix<process_id> {
    // TODO: `p_signal_info` should replace `nullptr`.
    return syscall<process_id>(247, type, pid, nullptr, options);
}
