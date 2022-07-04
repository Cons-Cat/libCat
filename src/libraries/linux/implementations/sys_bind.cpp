#include <cat/linux>

auto nix::sys_bind(nix::FileDescriptor const socket_descriptor,
                   void const* const p_socket, ssize const p_addr_len)
    -> nix::ScaredyLinux<void> {
    return nix::syscall<void>(49, socket_descriptor, p_socket, p_addr_len);
}
