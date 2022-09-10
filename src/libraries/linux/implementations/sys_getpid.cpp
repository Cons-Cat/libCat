#include <cat/linux>

auto nix::sys_getpid() -> ProcessId {
    return syscall<ProcessId>(39).value();
}
