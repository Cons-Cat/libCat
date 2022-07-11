#include <cat/linux>

auto nix::sys_accept(nix::FileDescriptor socket_descriptor,
                     void const* __restrict p_socket,
                     ssize const* __restrict p_addr_len)
    -> nix::ScaredyLinux<nix::FileDescriptor> {
    return nix::syscall<nix::FileDescriptor>(43, socket_descriptor, p_socket,
                                             p_addr_len);
}
