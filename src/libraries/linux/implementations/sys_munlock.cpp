#include <cat/linux>

auto
nix::sys_munlock(cat::span<cat::byte const> memory) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(150, memory.data(), memory.size());
}
