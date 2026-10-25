#include <cat/linux>

auto
nix::sys_ftruncate(file_descriptor file_descriptor, cat::iword length) noexcept
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(77, file_descriptor, length);
}
