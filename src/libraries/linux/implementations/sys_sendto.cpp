#include <cat/linux>

// Returns the number of characters sent to `p_destination_socket`.
auto
nix::sys_sendto(
   file_descriptor socket_descriptor, cat::str_view buffer, message_flags flags,
   cat::Socket const* _Nullable p_destination_socket, cat::iword addr_length
) -> nix::scaredy_nix<cat::iword> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::iword>(
      44, socket_descriptor, buffer.data(), buffer.size(), flags,
      p_destination_socket, addr_length
   );
}
