#include <cat/linux>

auto
nix::sys_unlink(char const* p_path_name) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(87, p_path_name);
}
