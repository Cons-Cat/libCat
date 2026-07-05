#include <cat/linux>

auto
nix::sys_statfs(cat::zstr_view file_path, statfs_data& out)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<void>(137, file_path.data(), &out);
}
