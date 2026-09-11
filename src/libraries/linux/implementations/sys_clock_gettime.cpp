#include <cat/linux>

auto
nix::sys_clock_gettime(clock_id clock, timespec& out)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(228, clock, &out);
}
