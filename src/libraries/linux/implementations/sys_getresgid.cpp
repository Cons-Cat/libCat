#include <cat/linux>

auto
nix::sys_getresgid(
   group_id& real, group_id& effective, group_id& saved
) noexcept -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<void>(120, &real, &effective, &saved);
}
