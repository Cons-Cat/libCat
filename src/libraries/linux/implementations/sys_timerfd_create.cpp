#include <cat/linux>

auto
nix::sys_timerfd_create(clock_id clock, timerfd_flags flags)
   -> nix::scaredy_nix<file_descriptor> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<file_descriptor>(283, clock, flags);
}
