// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::send_buffer(FileDescriptor const socket_descriptor,
                      void const* p_message_buffer, ssize const buffer_length,
                      int8 const flags, Socket const* p_destination_socket,
                      ssize const addr_length) -> Result<ssize> {
    return nix::syscall6(44u, socket_descriptor, p_message_buffer,
                         buffer_length, flags, p_destination_socket,
                         addr_length);
}
