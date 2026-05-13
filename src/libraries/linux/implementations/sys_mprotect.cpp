#include <cat/linux>

auto
nix::sys_mprotect(void* p_address, cat::uword length,
                  memory_protection_flags protections)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(10, p_address, length, protections);
}
