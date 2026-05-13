#include <cat/linux>

auto
nix::sys_shutdown(file_descriptor socket_descriptor, shutdown_how how)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(48, socket_descriptor, how);
}
