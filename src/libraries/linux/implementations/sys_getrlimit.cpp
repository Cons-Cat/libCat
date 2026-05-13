#include <cat/linux>

auto
nix::sys_getrlimit(rlimit_resource resource, rlimit* p_limits)
   -> scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return syscall<void>(97, resource, p_limits);
}
