#include <cat/linux>

auto
nix::sys_truncate(char const* p_file_path, cat::iword length)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(76, p_file_path, length);
}
