#include <cat/linux>

auto
nix::sys_map_shadow_stack(cat::uword address, cat::uword size, cat::uint4 flags)
   -> nix::scaredy_nix<void*> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void*>(453, address, size, flags);
}
