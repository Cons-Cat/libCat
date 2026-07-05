#include <cat/linux>

auto
nix::sys_chdir(cat::zstr_view file_path) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(80, file_path.data());
}
