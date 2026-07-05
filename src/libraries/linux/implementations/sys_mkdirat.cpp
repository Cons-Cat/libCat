#include <cat/linux>

auto
nix::sys_mkdirat(file_descriptor dirfd, cat::zstr_view file_path,
                 file_permissions mode) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(258, dirfd, file_path.data(), mode);
}
