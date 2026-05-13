#include <cat/linux>

auto
nix::sys_setresuid(user_id real, user_id effective, user_id saved)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(117, real, effective, saved);
}
