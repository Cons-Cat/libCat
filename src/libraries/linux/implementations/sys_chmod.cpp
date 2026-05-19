#include <cat/linux>

auto
nix::sys_chmod(char const* _Nonnull p_file_path, file_permissions mode)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(90, p_file_path, mode);
}
