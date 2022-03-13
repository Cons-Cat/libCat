// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>
#include <linux_flags>

auto nix::recieve_buffer(FileDescriptor const socket_descriptor,
                         void const* p_message_buffer,
                         isize const buffer_length,
                         Socket const* __restrict p_addr,
                         isize const* __restrict p_addr_length)
    -> Result<isize> {
    return nix::syscall5(45u, socket_descriptor, p_message_buffer,
                         buffer_length, p_addr, p_addr_length);
}
