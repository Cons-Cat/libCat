#include <cat/linux>

auto
nix::sys_arch_prctl(arch_prctl_request request, void* _Nonnull p_address)
   -> scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(158, request, p_address);
}
