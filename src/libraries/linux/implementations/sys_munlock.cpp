#include <cat/linux>

auto
nix::sys_munlock(void const* p_address, cat::uword length)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(150, p_address, length);
}
