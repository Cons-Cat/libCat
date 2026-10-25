#include <cat/linux>

auto
nix::sys_munlock(cat::span<cat::byte const> memory) noexcept
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(150, memory.data(), memory.size());
}
