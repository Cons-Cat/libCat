#include <cat/linux>

auto
nix::sys_fstatfs(file_descriptor file_descriptor, statfs_data& out)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<void>(138, file_descriptor, &out);
}
