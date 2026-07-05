#include <cat/linux>

// `nix::unmap_memory()` wraps the `munmap` Linux syscall.
auto
nix::sys_munmap(cat::span<cat::byte const> memory) -> nix::scaredy_nix<void> {
   poison_memory_region(memory.data(), memory.size());
   // https://filippo.io/linux-syscall-table/
   return nix::syscall_volatile<void>(11, memory.data(), memory.size());
}
