#include <cat/linux>

auto
nix::sys_flock(file_descriptor file_descriptor, flock_op operation)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(73, file_descriptor, operation);
}
