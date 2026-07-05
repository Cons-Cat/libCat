#include <cat/linux>

auto
nix::sys_getcwd(cat::span<char> buffer) -> nix::scaredy_nix<cat::idx> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<cat::idx>(79, buffer.data(), buffer.size());
}
