#include <cat/linux>

auto
nix::sys_newfstatat(file_descriptor dirfd, char const* p_file_path,
                    file_status& out, atfile_flags flags)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<void>(262, dirfd, p_file_path, &out, flags);
}
