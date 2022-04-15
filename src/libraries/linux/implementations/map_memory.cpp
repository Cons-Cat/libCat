// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <linux>

// `map_memory()` wraps the `mmap` Linux syscall. This returns the virtual
// memory address which it has allocated a page at.
auto nix::map_memory(usize const beginning_address, usize const bytes_size,
                     MemoryProtectionFlags const protections,
                     MemoryFlags const flags,
                     FileDescriptor const file_descriptor,
                     ssize const pages_offset) -> Result<void*> {
    return nix::syscall6(9u, beginning_address, bytes_size, protections, flags,
                         file_descriptor, pages_offset * page_size);
}
