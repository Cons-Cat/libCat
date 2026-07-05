#include <cat/linux>

auto
nix::sys_linkat(file_descriptor old_dirfd,
                cat::zstr_view existing_path,
                file_descriptor new_dirfd,
                cat::zstr_view new_path, atfile_flags flags)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(265, old_dirfd, existing_path.data(),
                                      new_dirfd, new_path.data(), flags);
}
