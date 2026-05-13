#include <cat/linux>

auto
nix::sys_arch_prctl(nix::arch_prctl_request const request,
                    void* const p_address) -> scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(158, request, p_address);
}
