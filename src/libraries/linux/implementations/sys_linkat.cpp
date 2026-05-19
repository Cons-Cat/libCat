#include <cat/linux>

auto
nix::sys_linkat(file_descriptor old_dirfd,
                char const* _Nonnull __restrict p_existing_path,
                file_descriptor new_dirfd,
                char const* _Nonnull __restrict p_new_path, atfile_flags flags)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(265, old_dirfd, p_existing_path,
                                      new_dirfd, p_new_path, flags);
}
