#include <cat/linux>

auto
nix::sys_clock_getres(clock_id clock, timespec& out) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<void>(229, clock, &out);
}
