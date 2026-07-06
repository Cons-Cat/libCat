#include <cat/linux>

// Connect a socket to an address.
auto
nix::sys_connect(
   file_descriptor socket_descriptor, void const* _Nonnull p_socket,
   cat::iword socket_size
) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(
      42, socket_descriptor, p_socket, socket_size
   );
}
