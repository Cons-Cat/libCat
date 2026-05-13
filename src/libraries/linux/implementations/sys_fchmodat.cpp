#include <cat/linux>

auto
nix::sys_fchmodat(file_descriptor dirfd, char const* p_file_path,
                  file_permissions mode, atfile_flags flags)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(268, dirfd, p_file_path, mode, flags);
}
