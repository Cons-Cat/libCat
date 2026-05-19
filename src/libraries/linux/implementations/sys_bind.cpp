#include <cat/linux>

auto
nix::sys_bind(file_descriptor socket_descriptor, void const* _Nonnull p_socket,
              cat::iword p_addr_len) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(49, socket_descriptor, p_socket,
                                      p_addr_len);
}
