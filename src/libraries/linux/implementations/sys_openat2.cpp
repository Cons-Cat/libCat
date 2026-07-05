#include <cat/linux>

auto
nix::sys_openat2(file_descriptor dirfd, cat::zstr_view file_path,
                 open_how const& how) -> nix::scaredy_nix<file_descriptor> {
   // The kernel takes `sizeof(open_how)` so it can detect callers built
   // against an older / newer struct layout.
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<file_descriptor>(437, dirfd, file_path.data(),
                                                 &how, sizeof(open_how));
}
