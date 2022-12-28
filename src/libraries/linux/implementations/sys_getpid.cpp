#include <cat/linux>

auto nix::sys_getpid() -> process_id {
    return syscall<process_id>(39).value();
}
