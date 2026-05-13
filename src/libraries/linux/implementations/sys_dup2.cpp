#include <cat/linux>

auto
nix::sys_dup2(file_descriptor oldfd, file_descriptor newfd)
   -> nix::scaredy_nix<file_descriptor> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<file_descriptor>(33, oldfd, newfd);
}
