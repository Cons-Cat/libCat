#include <cat/linux>

// `map_memory()` wraps the `mmap` Linux syscall. This returns the virtual
// memory address which it has allocated a page at.
auto nix::sys_mmap(uword beginning_address, iword bytes_size,
                   nix::memory_protection_flags protections,
                   nix::memory_flags flags, nix::file_descriptor file_descriptor,
                   iword pages_offset) -> nix::scaredy_nix<void*> {
    // TODO: Consider `__builtin_assume_aligned()`.
    return nix::syscall<void*>(9, beginning_address, bytes_size, protections,
                               flags, file_descriptor,
                               pages_offset * page_size);
}
