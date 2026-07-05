#include <cat/linux>

auto
nix::sys_renameat2(file_descriptor old_dirfd, cat::zstr_view old_path,
                   file_descriptor new_dirfd, cat::zstr_view new_path,
                   renameat2_flags flags) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(316, old_dirfd, old_path.data(),
                                      new_dirfd, new_path.data(), flags);
}
