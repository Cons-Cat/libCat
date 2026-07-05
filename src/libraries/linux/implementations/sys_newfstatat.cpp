#include <cat/linux>

auto
nix::sys_newfstatat(file_descriptor dirfd, cat::zstr_view file_path,
                    file_status& out, atfile_flags flags)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<void>(262, dirfd, file_path.data(), &out, flags);
}
