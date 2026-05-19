#include <cat/linux>

auto
nix::sys_renameat(file_descriptor old_dirfd,
                  char const* _Nonnull __restrict p_old_path,
                  file_descriptor new_dirfd,
                  char const* _Nonnull __restrict p_new_path)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(264, old_dirfd, p_old_path, new_dirfd,
                                      p_new_path);
}
