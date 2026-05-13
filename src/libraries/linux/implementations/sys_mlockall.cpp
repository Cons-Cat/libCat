#include <cat/linux>

auto
nix::sys_mlockall(mlockall_flags flags) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(151, flags);
}
