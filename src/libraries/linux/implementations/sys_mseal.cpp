#include <cat/linux>

auto
nix::sys_mseal(cat::span<cat::byte> memory, cat::uword flags)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(462, memory.data(), memory.size(), flags);
}
