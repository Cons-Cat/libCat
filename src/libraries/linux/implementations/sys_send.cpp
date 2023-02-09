#include <cat/linux>

// Returns the number of characters sent to `p_destination_socket`.
auto nix::sys_sendto(nix::file_descriptor socket_descriptor,
                     void const* p_message_buffer, iword buffer_length,
                     int8 flags, cat::Socket const* p_destination_socket,
                     iword addr_length) -> nix::scaredy_nix<iword> {
    return nix::syscall<iword>(44, socket_descriptor, p_message_buffer,
                               buffer_length, flags, p_destination_socket,
                               addr_length);
}
