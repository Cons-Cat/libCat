#include <cat/linux>

auto
nix::sys_mlock2(void const* p_address, cat::uword length, mlock2_flags flags)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(325, p_address, length, flags);
}
