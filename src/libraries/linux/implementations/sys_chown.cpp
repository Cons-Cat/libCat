#include <cat/linux>

auto
nix::sys_chown(char const* p_file_path, user_id user, group_id group)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(92, p_file_path, user, group);
}
