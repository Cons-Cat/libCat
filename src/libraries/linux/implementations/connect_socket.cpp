// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>
#include <linux_flags>

auto nix::connect_socket(FileDescriptor const socket_descriptor,
                         void const* p_socket, isize socket_size) -> Result<> {
    return nix::syscall3(42u, socket_descriptor, p_socket, socket_size);
}
