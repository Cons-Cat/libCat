#include <cat/linux>

auto
nix::sys_setpgid(process_id pid, process_id process_group)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(109, pid, process_group);
}
