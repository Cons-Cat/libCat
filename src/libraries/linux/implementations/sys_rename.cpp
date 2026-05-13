#include <cat/linux>

auto
nix::sys_rename(char const* p_old_path, char const* p_new_path)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(82, p_old_path, p_new_path);
}
