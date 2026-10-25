#include <cat/linux>

auto
nix::sys_fchmod(file_descriptor file_descriptor, file_permissions mode) noexcept
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(91, file_descriptor, mode);
}
