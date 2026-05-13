#include <cat/linux>

auto
nix::sys_fsync(file_descriptor file_descriptor) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(74, file_descriptor);
}
