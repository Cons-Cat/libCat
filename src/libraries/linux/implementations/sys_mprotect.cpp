#include <cat/linux>

auto
nix::sys_mprotect(
   cat::span<cat::byte const> memory, memory_protection_flags protections
) -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(
      10, memory.data(), memory.size(), protections
   );
}
