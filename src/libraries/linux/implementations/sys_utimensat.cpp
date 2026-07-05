#include <cat/linux>

auto
nix::sys_utimensat(file_descriptor dirfd, cat::zstr_view file_path,
                   futex_timespec const (*_Nullable p_times)[2],
                   atfile_flags flags) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(280, dirfd, file_path.data(), p_times,
                                      flags);
}
