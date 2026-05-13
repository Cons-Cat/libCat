#include <cat/linux>

auto
nix::sys_dup(file_descriptor fd) -> nix::scaredy_nix<file_descriptor> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<file_descriptor>(32, fd);
}
