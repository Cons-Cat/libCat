#include <cat/linux>

auto
nix::sys_dup3(file_descriptor oldfd, file_descriptor newfd, dup3_flags flags)
   -> nix::scaredy_nix<file_descriptor> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<file_descriptor>(292, oldfd, newfd, flags);
}
