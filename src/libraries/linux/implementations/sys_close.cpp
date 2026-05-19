#include <cat/linux>

auto
nix::sys_close(file_descriptor descriptor) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(3, descriptor);
}
