#include <cat/linux>

auto
nix::sys_mlock(cat::span<cat::byte const> memory) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(149, memory.data(), memory.size());
}
