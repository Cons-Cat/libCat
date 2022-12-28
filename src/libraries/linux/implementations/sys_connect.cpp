#include <cat/linux>

// Connect a socket to an address.
auto nix::sys_connect(nix::file_descriptor socket_descriptor,
                      void const* p_socket, ssize socket_size)
    -> nix::scaredy_nix<void> {
    return nix::syscall<void>(42, socket_descriptor, p_socket, socket_size);
}
