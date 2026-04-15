#include <cat/linux>

auto
nix::sys_arch_prctl(nix::arch_prctl_request const request,
                    void* const p_address) -> scaredy_nix<void> {
   return nix::syscall<void>(158, request, p_address);
}
