#include <cat/linux>

// Make a connection over a `cat::Socket`. This returns a new socket which has
// been connected to. This new `cat::Socket` is not in a listening state.
auto nix::sys_accept(nix::file_descriptor socket_descriptor,
                     void const* __restrict p_socket,
                     iword const* __restrict p_addr_len)
    -> nix::scaredy_nix<nix::file_descriptor> {
    return nix::syscall<nix::file_descriptor>(43, socket_descriptor, p_socket,
                                             p_addr_len);
}
