#include <cat/linux>

auto
nix::sys_getresuid(user_id* p_real, user_id* p_effective, user_id* p_saved)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<void>(118, p_real, p_effective, p_saved);
}
