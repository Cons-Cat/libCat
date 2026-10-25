#include <cat/linux>

auto
nix::sys_umask(file_permissions mask) noexcept
   -> nix::scaredy_nix<file_permissions> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<file_permissions>(95, mask);
}
