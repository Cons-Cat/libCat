#include <cat/linux>

auto
nix::sys_access(char const* p_file_path, access_mode mode)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<void>(21, p_file_path, mode);
}
