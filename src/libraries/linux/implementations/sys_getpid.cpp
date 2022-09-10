#include <cat/linux>

void nix::sys_getpid() {
    _ = syscall0(39);
}
