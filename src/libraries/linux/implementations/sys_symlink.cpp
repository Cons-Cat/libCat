#include <cat/linux>

auto
nix::sys_symlink(cat::zstr_view target_path,
                 cat::zstr_view link_path)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(88, target_path.data(), link_path.data());
}
