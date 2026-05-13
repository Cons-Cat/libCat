#include <cat/linux>

auto
nix::sys_getresgid(group_id* p_real, group_id* p_effective, group_id* p_saved)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<void>(120, p_real, p_effective, p_saved);
}
