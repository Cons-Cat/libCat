#include <cat/linux>

auto
nix::sys_munlockall() noexcept -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(152);
}
