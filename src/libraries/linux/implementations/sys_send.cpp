#include <cat/linux>

// Returns the number of characters sent to `p_destination_socket`.
auto nix::sys_sendto(nix::file_descriptor socket_descriptor,
                     void const* p_message_buffer, ssize buffer_length,
                     int8 flags, cat::Socket const* p_destination_socket,
                     ssize addr_length) -> nix::scaredy_nix<ssize> {
    return nix::syscall<ssize>(44, socket_descriptor, p_message_buffer,
                               buffer_length, flags, p_destination_socket,
                               addr_length);
}
