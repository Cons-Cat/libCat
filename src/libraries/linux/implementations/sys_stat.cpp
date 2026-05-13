#include <cat/linux>

auto
nix::sys_stat(cat::str_view const file_path, file_status& out)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return syscall<void>(4, file_path.data(), &out);
}
