#include <cat/linux>

auto
nix::sys_link(cat::zstr_view existing_path, cat::zstr_view new_path)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(
      86, existing_path.data(), new_path.data()
   );
}
