#include <cat/linux>

auto
nix::sys_getrandom(cat::span<unsigned char> buffer, getrandom_flags flags)
   -> nix::scaredy_nix<cat::idx> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::idx>(
      318, buffer.data(), buffer.size(), flags
   );
}
