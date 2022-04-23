// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::recieve_buffer(FileDescriptor const socket_descriptor,
                         void const* p_message_buffer,
                         ssize const buffer_length,
                         Socket const* __restrict p_addr,
                         ssize const* __restrict p_addr_length)
    -> Result<ssize> {
    return nix::syscall5(45, socket_descriptor, p_message_buffer, buffer_length,
                         p_addr, p_addr_length);
}
