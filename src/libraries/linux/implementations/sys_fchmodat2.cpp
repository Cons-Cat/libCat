#include <cat/linux>

auto
nix::sys_fchmodat2(file_descriptor dirfd, char const* p_path, cat::uint4 mode,
                   cat::int4 flags) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(452, dirfd, p_path, mode, flags);
}
