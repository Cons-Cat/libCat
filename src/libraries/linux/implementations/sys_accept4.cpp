#include <cat/linux>

auto
nix::sys_accept4(file_descriptor socket_descriptor,
                 cat::Socket* __restrict p_socket,
                 cat::iword* __restrict p_addr_len, accept4_flags flags)
   -> nix::scaredy_nix<file_descriptor> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<file_descriptor>(288, socket_descriptor,
                                                 p_socket, p_addr_len, flags);
}
