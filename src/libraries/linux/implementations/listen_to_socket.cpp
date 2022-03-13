// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>
#include <linux_flags>

auto nix::listen_to_socket(FileDescriptor const socket_descriptor,
                           int8 const backlog) -> Result<> {
    return nix::syscall2(50u, socket_descriptor, backlog);
}
