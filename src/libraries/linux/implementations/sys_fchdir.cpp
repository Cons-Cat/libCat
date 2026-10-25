#include <cat/linux>

auto
nix::sys_fchdir(file_descriptor file_descriptor) noexcept
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(81, file_descriptor);
}
