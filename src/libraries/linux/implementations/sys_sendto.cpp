#include <cat/linux>

// Returns the number of characters sent to `p_destination_socket`.
auto
nix::sys_sendto(file_descriptor socket_descriptor,
                void const* _Nonnull p_message_buffer, cat::iword buffer_length,
                cat::int8 flags,
                cat::Socket const* _Nullable p_destination_socket,
                cat::iword addr_length) -> nix::scaredy_nix<cat::iword> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::iword>(
      44, socket_descriptor, p_message_buffer, buffer_length, flags,
      p_destination_socket, addr_length);
}
