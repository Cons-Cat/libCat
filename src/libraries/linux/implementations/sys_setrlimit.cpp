#include <cat/linux>

auto
nix::sys_setrlimit(rlimit_resource resource, rlimit const* p_limits)
   -> scaredy_nix<void> {
   return syscall<void>(160, resource, p_limits);
}
