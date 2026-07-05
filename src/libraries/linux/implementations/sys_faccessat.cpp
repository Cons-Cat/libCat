#include <cat/linux>

auto
nix::sys_faccessat(file_descriptor dirfd, cat::zstr_view file_path,
                   access_mode mode, atfile_flags flags)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<void>(269, dirfd, file_path.data(), mode, flags);
}
