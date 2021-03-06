#include <cat/linux>

auto nix::sys_listen(nix::FileDescriptor socket_descriptor, int8 backlog)
    -> nix::ScaredyLinux<void> {
    return nix::syscall<void>(50, socket_descriptor, backlog);
}
