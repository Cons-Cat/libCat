// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <unistd.h>

enum ProtectionFlags : u4
{
    PROT_NONE = 0b000,   // Data cannot be accessed at all.
    PROT_READ = 0b001,   // Data is readable.
    PROT_WRITE = 0b010,  // Data is writable.
    PROT_EXEC = 0b100,   // Data can be executed.
};

enum VisibilityFlags : u4
{
    MAP_SHARED = 0b1,    // Writes change the underlying object.
    MAP_PRIVATE = 0b10,  // Writes only change the calling process.
    MAP_FIXED = 0b10000, /* Map to precisely this address, rather than virtual
                          * memory. This may fail. */
    MAP_ANONYMOUS = 0b100000, /* TODO */
};

constexpr usize page_size = 4096u;

// TODO: Create an enum for unique failure codes.
/* `map_memory()` represents the mmap Linux syscall. It takes unsigned
 * arguments, as per the Linux ABI. It returns the address which it has
 * allocated at. */
auto map_memory(usize beginning_address, usize bytes_size,
                ProtectionFlags protections, VisibilityFlags flags,
                i4 file_descriptor, usize byte_offset) -> Result<void*> {
    // Pages are allocated in 4 kibibyte blocks.

    // Fail if the `byte_offset` is not a multipe of a page size.
    if ((byte_offset & (page_size - 1)) != 0) {
        return Failure(1);
    }

    // Fail if the `bytes_size` is 0.
    if (bytes_size == 0) {
        return Failure(1);
    }

    return syscall<void*>(9, beginning_address, bytes_size, protections, flags,
                          file_descriptor, byte_offset);
}

auto unmap_memory() -> Result<> {
    return okay;
}
