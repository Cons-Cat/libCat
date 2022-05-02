// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::listen_to_socket(nix::FileDescriptor const socket_descriptor,
                           int8 const backlog) -> nix::ScaredyLinux<void> {
    return nix::syscall<void>(50, socket_descriptor, backlog);
}
