#include <cat/linux>

// Mark a socket as available to make connections with `sys_accept()`.
auto nix::sys_listen(nix::file_descriptor socket_descriptor, cat::int8 backlog)
    -> nix::scaredy_nix<void> {
    return nix::syscall<void>(50, socket_descriptor, backlog);
}
