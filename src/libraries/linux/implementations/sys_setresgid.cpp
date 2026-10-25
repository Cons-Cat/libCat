#include <cat/linux>

auto
nix::sys_setresgid(group_id real, group_id effective, group_id saved) noexcept
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(119, real, effective, saved);
}
