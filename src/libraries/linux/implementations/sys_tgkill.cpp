#include <cat/linux>

auto
nix::sys_tgkill(process_id thread_group_id, process_id target, signal delivered)
   -> scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return syscall_volatile<void>(234, thread_group_id, target, delivered);
}
