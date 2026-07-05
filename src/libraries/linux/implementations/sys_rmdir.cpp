#include <cat/linux>

auto
nix::sys_rmdir(cat::zstr_view file_path) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(84, file_path.data());
}
