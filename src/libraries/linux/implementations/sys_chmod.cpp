#include <cat/linux>

auto
nix::sys_chmod(cat::zstr_view file_path, file_permissions mode)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(90, file_path.data(), mode);
}
