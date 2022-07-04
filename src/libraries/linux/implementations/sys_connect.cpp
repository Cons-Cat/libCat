#include <cat/linux>

auto nix::sys_connect(nix::FileDescriptor const socket_descriptor,
                      void const* const p_socket, ssize const socket_size)
    -> nix::ScaredyLinux<void> {
    return nix::syscall<void>(42, socket_descriptor, p_socket, socket_size);
}
