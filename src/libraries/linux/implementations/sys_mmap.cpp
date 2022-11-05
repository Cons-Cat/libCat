#include <cat/linux>

// `map_memory()` wraps the `mmap` Linux syscall. This returns the virtual
// memory address which it has allocated a page at.
auto nix::sys_mmap(usize beginning_address, ssize bytes_size,
                   nix::MemoryProtectionFlags protections,
                   nix::MemoryFlags flags, nix::FileDescriptor file_descriptor,
                   ssize pages_offset) -> nix::ScaredyLinux<void*> {
    // TODO: Consider `__builtin_assume_aligned()`.
    return nix::syscall<void*>(9, beginning_address, bytes_size, protections,
                               flags, file_descriptor,
                               pages_offset * page_size);
}
