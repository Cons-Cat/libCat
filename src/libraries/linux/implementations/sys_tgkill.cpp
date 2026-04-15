#include <cat/linux>

auto
nix::sys_tgkill(process_id thread_group_id, process_id target, signal delivered)
   -> scaredy_nix<void> {
   return syscall<void>(234, thread_group_id, target, delivered);
}
