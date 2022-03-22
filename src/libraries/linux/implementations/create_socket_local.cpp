// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::create_socket_local(int8 const type, int8 const protocol)
    -> Result<FileDescriptor> {
    return nix::create_socket(1, type, protocol);
}
