#include <cat/linux>

auto
nix::sys_setuid(user_id user) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(105, user);
}
