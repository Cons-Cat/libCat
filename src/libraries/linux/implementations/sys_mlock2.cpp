#include <cat/linux>

auto
nix::sys_mlock2(cat::span<cat::byte const> memory, mlock2_flags flags)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(325, memory.data(), memory.size(), flags);
}
