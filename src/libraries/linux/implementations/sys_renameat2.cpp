#include <cat/linux>

auto
nix::sys_renameat2(file_descriptor old_dirfd, char const* p_old_path,
                   file_descriptor new_dirfd, char const* p_new_path,
                   renameat2_flags flags) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(316, old_dirfd, p_old_path, new_dirfd,
                                      p_new_path, flags);
}
