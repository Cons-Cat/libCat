#include <cat/linux>

auto
nix::sys_statx(file_descriptor dirfd, cat::zstr_view file_path,
               atfile_flags flags, statx_mask mask, statx_data& out)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<void>(332, dirfd, file_path.data(), flags, mask, &out);
}
