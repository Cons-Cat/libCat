#include <cat/linux>

auto
nix::sys_chdir(char const* _Nonnull p_file_path) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(80, p_file_path);
}
