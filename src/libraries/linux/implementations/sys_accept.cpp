#include <cat/linux>

// Make a connection over a `cat::Socket`. This returns a new socket which has
// been connected to. This new `cat::Socket` is not in a listening state.
auto nix::sys_accept(nix::FileDescriptor socket_descriptor,
                     void const* __restrict p_socket,
                     ssize const* __restrict p_addr_len)
    -> nix::ScaredyLinux<nix::FileDescriptor> {
    return nix::syscall<nix::FileDescriptor>(43, socket_descriptor, p_socket,
                                             p_addr_len);
}
