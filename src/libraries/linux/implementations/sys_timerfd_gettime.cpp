#include <cat/linux>

auto
nix::sys_timerfd_gettime(file_descriptor file_descriptor, itimerspec& out)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall<void>(287, file_descriptor, &out);
}
