#include <cat/linux>

auto nix::sys_bind(nix::FileDescriptor socket_descriptor, void const* p_socket,
                   ssize p_addr_len) -> nix::ScaredyLinux<void> {
    return nix::syscall<void>(49, socket_descriptor, p_socket, p_addr_len);
}
