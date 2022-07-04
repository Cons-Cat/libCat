#include <cat/linux>

auto nix::sys_close(nix::FileDescriptor const object)
    -> nix::ScaredyLinux<void> {
    return nix::syscall<void>(3, object);
}
