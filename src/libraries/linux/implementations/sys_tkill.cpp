#include <cat/linux>

auto nix::sys_tkill(ProcessId pid, Signal signal) -> ScaredyLinux<void> {
    return syscall<void>(200, pid, signal);
}
