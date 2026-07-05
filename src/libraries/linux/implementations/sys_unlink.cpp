#include <cat/linux>

auto
nix::sys_unlink(cat::zstr_view path_name) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(87, path_name.data());
}
