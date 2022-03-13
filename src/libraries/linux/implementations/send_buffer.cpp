// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>
#include <linux_flags>

auto nix::send_buffer(FileDescriptor const socket_descriptor,
                      void const* p_message_buffer, isize const buffer_length,
                      int4 const flags, Socket const* p_destination_socket,
                      isize const addr_length) -> Result<isize> {
    return nix::syscall6(44u, socket_descriptor, p_message_buffer,
                         buffer_length, flags, p_destination_socket,
                         addr_length);
}
