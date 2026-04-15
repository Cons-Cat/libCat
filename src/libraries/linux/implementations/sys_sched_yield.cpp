#include <cat/linux>

auto
nix::sys_sched_yield() -> scaredy_nix<void> {
   return syscall<void>(24);
}
