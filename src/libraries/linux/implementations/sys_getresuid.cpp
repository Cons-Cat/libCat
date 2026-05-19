#include <cat/linux>

auto
nix::sys_getresuid(user_id& real, user_id& effective, user_id& saved)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<void>(118, &real, &effective, &saved);
}
