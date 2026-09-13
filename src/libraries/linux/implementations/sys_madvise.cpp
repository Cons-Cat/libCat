#include <cat/linux>

auto
nix::sys_madvise(cat::span<cat::byte const> memory, madvise_advice advice)
   -> nix::scaredy_nix<void> {
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(28, memory.data(), memory.size(), advice);
}
