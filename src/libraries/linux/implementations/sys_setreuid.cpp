#include <cat/linux>

auto
nix::sys_setreuid(user_id real, user_id effective) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(113, real, effective);
}
