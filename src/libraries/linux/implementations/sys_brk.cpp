#include <cat/linux>

auto
nix::sys_brk(void* p_address) -> nix::scaredy_nix<void*> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void*>(12, p_address);
}
