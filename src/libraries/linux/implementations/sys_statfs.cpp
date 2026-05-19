#include <cat/linux>

auto
nix::sys_statfs(char const* _Nonnull p_file_path, statfs_data& out)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<void>(137, p_file_path, &out);
}
