// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::listen_to_socket(FileDescriptor const socket_descriptor,
                           int8 const backlog) -> Result<> {
    return nix::syscall2(50, socket_descriptor, backlog);
}
