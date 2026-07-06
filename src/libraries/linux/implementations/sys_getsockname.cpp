#include <cat/linux>

auto
nix::sys_getsockname(
   file_descriptor socket_descriptor, cat::Socket& out_socket,
   cat::iword& inout_addr_length
) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<void>(
      51, socket_descriptor, &out_socket, &inout_addr_length
   );
}
