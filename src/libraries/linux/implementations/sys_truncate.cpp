#include <cat/linux>

auto
nix::sys_truncate(cat::zstr_view file_path, cat::iword length)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(76, file_path.data(), length);
}
