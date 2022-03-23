// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

// TODO: `unmap_memory()` wraps the `munmap` Linux syscall.
auto nix::unmap_memory() -> Result<> {
    return okay;
}
