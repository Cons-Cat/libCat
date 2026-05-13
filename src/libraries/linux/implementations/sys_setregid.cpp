#include <cat/linux>

auto
nix::sys_setregid(group_id real, group_id effective) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(114, real, effective);
}
