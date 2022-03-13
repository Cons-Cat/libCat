// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>
#include <linux_flags>

auto nix::create_socket(int8 const protocol_family, int8 const type,
                        int8 const protocol) -> Result<FileDescriptor> {
    return nix::syscall3(41u, protocol_family, type, protocol);
}
