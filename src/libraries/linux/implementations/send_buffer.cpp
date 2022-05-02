// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

auto nix::send_buffer(nix::FileDescriptor const socket_descriptor,
                      void const* p_message_buffer, ssize const buffer_length,
                      int8 const flags, cat::Socket const* p_destination_socket,
                      ssize const addr_length) -> nix::ScaredyLinux<ssize> {
    return nix::syscall<ssize>(44, socket_descriptor, p_message_buffer,
                               buffer_length, flags, p_destination_socket,
                               addr_length);
}
