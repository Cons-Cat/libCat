// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::connect_socket(FileDescriptor const socket_descriptor,
                         void const* p_socket, ssize socket_size) -> Result<> {
    return nix::syscall3(42, socket_descriptor, p_socket, socket_size);
}
