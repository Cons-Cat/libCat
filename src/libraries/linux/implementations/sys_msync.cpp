#include <cat/linux>

auto
nix::sys_msync(cat::span<cat::byte const> memory, msync_flags flags)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(26, memory.data(), memory.size(), flags);
}
