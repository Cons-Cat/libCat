#include <cat/linux>

auto
nix::sys_mkdir(char const* _Nonnull p_file_path, file_permissions mode)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(83, p_file_path, mode);
}
