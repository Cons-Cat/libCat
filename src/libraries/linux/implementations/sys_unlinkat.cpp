#include <cat/linux>

auto
nix::sys_unlinkat(file_descriptor dirfd, char const* _Nonnull p_file_path,
                  atfile_flags flags) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(263, dirfd, p_file_path, flags);
}
