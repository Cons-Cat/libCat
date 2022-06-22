#include <cat/linux>

auto nix::accept_socket(nix::FileDescriptor const socket_descriptor,
                        void const* const __restrict p_socket,
                        ssize const* const __restrict p_addr_len)
    -> nix::ScaredyLinux<nix::FileDescriptor> {
    return nix::syscall<nix::FileDescriptor>(43, socket_descriptor, p_socket,
                                             p_addr_len);
}
