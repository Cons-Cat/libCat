#include <cat/linux>

auto
nix::sys_setgid(group_id group) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(106, group);
}
