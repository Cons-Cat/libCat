#include <cat/linux>

auto
nix::sys_faccessat(file_descriptor dirfd, char const* p_file_path,
                   access_mode mode, atfile_flags flags)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<void>(269, dirfd, p_file_path, mode, flags);
}
