#include <cat/linux>

auto
nix::sys_symlink(char const* p_target_path, char const* p_link_path)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(88, p_target_path, p_link_path);
}
