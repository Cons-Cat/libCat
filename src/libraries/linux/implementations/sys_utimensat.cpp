#include <cat/linux>

auto
nix::sys_utimensat(file_descriptor dirfd, char const* p_file_path,
                   futex_timespec const (*p_times)[2], atfile_flags flags)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(280, dirfd, p_file_path, p_times, flags);
}
