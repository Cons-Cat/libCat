#include <cat/linux>

auto
nix::sys_fchmodat(
   file_descriptor dirfd, cat::zstr_view file_path, file_permissions mode,
   atfile_flags flags
) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(
      268, dirfd, file_path.data(), mode, flags
   );
}
