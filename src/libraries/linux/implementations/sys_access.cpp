#include <cat/linux>

auto
nix::sys_access(cat::zstr_view file_path, access_mode mode)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<void>(21, file_path.data(), mode);
}
