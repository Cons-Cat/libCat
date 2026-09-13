#include <cat/linux>

auto
nix::sys_getpeername(
   file_descriptor socket_descriptor, void* _Nonnull p_out_socket,
   cat::iword& inout_addr_length
) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<void>(
      52, socket_descriptor, p_out_socket, &inout_addr_length
   );
}
