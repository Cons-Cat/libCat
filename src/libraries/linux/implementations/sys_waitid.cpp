#include <cat/linux>

auto nix::sys_waitid(WaitId type, ProcessId pid, WaitOptionsFlags options)
    -> ScaredyLinux<ProcessId> {
    // TODO: `p_signal_info` should replace `nullptr`.
    return syscall<ProcessId>(247, type, pid, nullptr, options);
}
