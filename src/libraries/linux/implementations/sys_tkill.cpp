#include <cat/linux>

auto nix::sys_tkill(process_id pid, Signal signal) -> scaredy_nix<void> {
    return syscall<void>(200, pid, signal);
}
