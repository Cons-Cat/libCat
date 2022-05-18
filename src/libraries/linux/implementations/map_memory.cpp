#include <cat/linux>

// `map_memory()` wraps the `mmap` Linux syscall. This returns the virtual
// memory address which it has allocated a page at.
auto nix::map_memory(usize const beginning_address, ssize const bytes_size,
                     nix::MemoryProtectionFlags const protections,
                     nix::MemoryFlags const flags,
                     nix::FileDescriptor const file_descriptor,
                     ssize const pages_offset) -> nix::ScaredyLinux<void*> {
    // TODO: Consider `__builtin_assume_aligned()`.
    // TODO: Simplify this.
    nix::ScaredyLinux<void*> result =
        nix::syscall<void*>(9, beginning_address, bytes_size, protections,
                            flags, file_descriptor, pages_offset * page_size);
    return result;
}
