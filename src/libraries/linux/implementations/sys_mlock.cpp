#include <cat/linux>

auto
nix::sys_mlock(void const* p_address, cat::uword length)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(149, p_address, length);
}
