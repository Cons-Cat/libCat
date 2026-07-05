#include <cat/linux>

auto
nix::sys_fchmodat2(file_descriptor dirfd, cat::zstr_view path, cat::uint4 mode,
                   cat::int4 flags) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(452, dirfd, path.data(), mode, flags);
}
