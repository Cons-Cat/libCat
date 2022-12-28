#include <cat/linux>

// `nix::unmap_memory()` wraps the `munmap` Linux syscall.
auto nix::sys_munmap(void const* p_memory, ssize length)
    -> nix::scaredy_nix<void> {
    return nix::syscall<void>(11, p_memory, length);
}
