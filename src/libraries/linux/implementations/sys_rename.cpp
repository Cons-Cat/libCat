#include <cat/linux>

auto
nix::sys_rename(cat::zstr_view old_path, cat::zstr_view new_path)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(82, old_path.data(), new_path.data());
}
