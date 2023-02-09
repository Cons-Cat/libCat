#include <cat/linux>

auto nix::sys_bind(nix::file_descriptor socket_descriptor, void const* p_socket,
                   iword p_addr_len) -> nix::scaredy_nix<void> {
    return nix::syscall<void>(49, socket_descriptor, p_socket, p_addr_len);
}
