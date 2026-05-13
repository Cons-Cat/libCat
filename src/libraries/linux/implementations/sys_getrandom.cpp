#include <cat/linux>

auto
nix::sys_getrandom(void* p_buffer, cat::iword length, getrandom_flags flags)
   -> nix::scaredy_nix<cat::iword> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::iword>(318, p_buffer, length, flags);
}
