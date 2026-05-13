#include <cat/linux>

auto
nix::sys_getpeername(file_descriptor socket_descriptor, cat::Socket* p_socket,
                     cat::iword* p_addr_length) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<void>(52, socket_descriptor, p_socket, p_addr_length);
}
