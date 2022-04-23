// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::accept_socket(FileDescriptor const socket_descriptor,
                        void const* __restrict p_socket,
                        ssize const* __restrict p_addr_len)
    -> Result<FileDescriptor> {
    return nix::syscall3(43, socket_descriptor, p_socket, p_addr_len);
}
