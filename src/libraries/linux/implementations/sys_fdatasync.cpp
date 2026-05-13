#include <cat/linux>

auto
nix::sys_fdatasync(file_descriptor file_descriptor) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(75, file_descriptor);
}
