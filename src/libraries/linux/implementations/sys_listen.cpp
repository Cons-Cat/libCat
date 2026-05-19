#include <cat/linux>

// Mark a socket as available to make connections with `sys_accept()`.
auto
nix::sys_listen(file_descriptor socket_descriptor, cat::int8 backlog)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(50, socket_descriptor, backlog);
}
