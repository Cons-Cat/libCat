#include <cat/linux>

auto
nix::sys_mseal(void* _Nonnull p_address, cat::uword length, cat::uword flags)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(462, p_address, length, flags);
}
