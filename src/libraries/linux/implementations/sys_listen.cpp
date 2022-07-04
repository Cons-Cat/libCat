#include <cat/linux>

auto nix::sys_listen(nix::FileDescriptor const socket_descriptor,
                     int8 const backlog) -> nix::ScaredyLinux<void> {
    return nix::syscall<void>(50, socket_descriptor, backlog);
}
