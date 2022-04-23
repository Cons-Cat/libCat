// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::bind_socket(FileDescriptor const socket_descriptor,
                      void const* p_socket, ssize const p_addr_len)
    -> Result<> {
    return nix::syscall3(49, socket_descriptor, p_socket, p_addr_len);
}
