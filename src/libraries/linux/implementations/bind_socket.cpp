#include <cat/linux>

auto nix::bind_socket(nix::FileDescriptor const socket_descriptor,
                      void const* p_socket, ssize const p_addr_len)
    -> nix::ScaredyLinux<void> {
    return nix::syscall<void>(49, socket_descriptor, p_socket, p_addr_len);
}
