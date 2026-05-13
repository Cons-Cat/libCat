#include <cat/linux>

auto
nix::sys_lstat(cat::str_view const file_path, file_status& out)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return syscall<void>(6, file_path.data(), &out);
}
