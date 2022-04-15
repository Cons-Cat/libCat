// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

// `nix::unmap_memory()` wraps the `munmap` Linux syscall.
auto nix::unmap_memory(void const* const p_memory, ssize const length)
    -> Result<> {
    return nix::syscall2(11, p_memory, length);
}
