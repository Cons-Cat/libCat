#include <cat/linux>

// `nix::unmap_memory()` wraps the `munmap` Linux syscall.
auto
nix::sys_munmap(void const* p_memory, cat::uword length)
   -> nix::scaredy_nix<void> {
   poison_memory_region(p_memory, length);
   return nix::syscall<void>(11, p_memory, length);
}
