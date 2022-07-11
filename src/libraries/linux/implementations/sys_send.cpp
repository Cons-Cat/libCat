#include <cat/linux>

auto nix::sys_send(nix::FileDescriptor socket_descriptor,
                   void const* p_message_buffer, ssize buffer_length,
                   int8 flags, cat::Socket const* p_destination_socket,
                   ssize addr_length) -> nix::ScaredyLinux<ssize> {
    return nix::syscall<ssize>(44, socket_descriptor, p_message_buffer,
                               buffer_length, flags, p_destination_socket,
                               addr_length);
}
