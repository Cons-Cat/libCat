#include <cat/linux>

// Connect a socket to an address.
auto nix::sys_connect(nix::FileDescriptor socket_descriptor,
                      void const* p_socket, ssize socket_size)
    -> nix::ScaredyLinux<void> {
    return nix::syscall<void>(42, socket_descriptor, p_socket, socket_size);
}
