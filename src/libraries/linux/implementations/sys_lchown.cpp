#include <cat/linux>

auto
nix::sys_lchown(cat::zstr_view file_path, user_id user, group_id group)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(94, file_path.data(), user, group);
}
