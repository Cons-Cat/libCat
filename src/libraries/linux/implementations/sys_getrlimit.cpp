#include <cat/linux>

auto
nix::sys_getrlimit(rlimit_resource resource, rlimit& out_limits)
   -> scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return syscall<void>(97, resource, &out_limits);
}
