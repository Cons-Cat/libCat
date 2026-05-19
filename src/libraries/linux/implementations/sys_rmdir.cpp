#include <cat/linux>

auto
nix::sys_rmdir(char const* _Nonnull p_file_path) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(84, p_file_path);
}
