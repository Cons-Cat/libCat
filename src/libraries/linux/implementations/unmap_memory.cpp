#include <cat/linux>

// `nix::unmap_memory()` wraps the `munmap` Linux syscall.
auto nix::unmap_memory(void const* const p_memory, ssize const length)
    -> nix::ScaredyLinux<void> {
    return nix::syscall<void>(11, p_memory, length);
}
