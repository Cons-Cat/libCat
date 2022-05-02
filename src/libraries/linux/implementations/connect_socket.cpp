// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::connect_socket(nix::FileDescriptor const socket_descriptor,
                         void const* p_socket, ssize socket_size)
    -> nix::ScaredyLinux<void> {
    return nix::syscall<void>(42, socket_descriptor, p_socket, socket_size);
}
