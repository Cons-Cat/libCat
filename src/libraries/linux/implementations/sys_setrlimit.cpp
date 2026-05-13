#include <cat/linux>

auto
nix::sys_setrlimit(rlimit_resource resource, rlimit const* p_limits)
   -> scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return syscall_volatile<void>(160, resource, p_limits);
}
