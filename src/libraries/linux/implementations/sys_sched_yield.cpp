#include <cat/linux>

auto
nix::sys_sched_yield() -> scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return syscall_volatile<void>(24);
}
