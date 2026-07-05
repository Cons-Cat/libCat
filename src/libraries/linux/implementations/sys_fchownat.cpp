#include <cat/linux>

auto
nix::sys_fchownat(file_descriptor dirfd, cat::zstr_view file_path, user_id user,
                  group_id group, atfile_flags flags)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(260, dirfd, file_path.data(), user, group,
                                      flags);
}
