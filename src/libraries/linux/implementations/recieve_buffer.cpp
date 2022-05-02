// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::recieve_buffer(nix::FileDescriptor const socket_descriptor,
                         void const* p_message_buffer,
                         ssize const buffer_length,
                         cat::Socket const* __restrict p_addr,
                         ssize const* __restrict p_addr_length)
    -> nix::ScaredyLinux<ssize> {
    return nix::syscall<ssize>(45, socket_descriptor, p_message_buffer,
                               buffer_length, p_addr, p_addr_length);
}
