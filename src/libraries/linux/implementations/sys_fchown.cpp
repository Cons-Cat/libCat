#include <cat/linux>

auto
nix::sys_fchown(file_descriptor file_descriptor, user_id user, group_id group)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(93, file_descriptor, user, group);
}
