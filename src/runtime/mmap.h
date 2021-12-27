// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// TODO: Create an enum for unique failure codes.
#include <unistd.h>

enum ProtectionFlags : u4
{
    PROT_NONE = 0b000,   // Data cannot be accessed at all.
    PROT_READ = 0b001,   // Data is readable.
    PROT_WRITE = 0b010,  // Data is writable.
    PROT_EXEC = 0b100,   // Data can be executed.
};

enum Flags : u4
{
    MAP_SHARED = 0b1,    // Writes change the underlying object.
    MAP_PRIVATE = 0b10,  // Writes only change the calling process.
    MAP_FIXED = 0b10000, /* Map to precisely this address, rather than virtual
                          * memory. This may fail. */
    MAP_ANONYMOUS = 0b100000,  // Not backed by a file descriptor.
    // TODO: Make binary/hexa format consistent.
    MAP_GROWSDOWN = 0x00100,   // Stack-like segment.
    MAP_DENYWRITE = 0x00800,   // ETXTBSY.
    MAP_EXECUTABLE = 0x01000,  // Mark it as an executable.
    MAP_LOCKED = 0x02000,      // Lock the mapping.
    MAP_NORESERVE = 0x04000,   // Don't check for reservations.
    MAP_POPULATE = 0x08000,    // Populate (prefault) pagetables.
    MAP_NONBLOCK = 0x10000,    // Do not block on IO.
    MAP_STACK = 0x20000,       // Allocation is for a stack.
    MAP_HUGETLB = 0x40000,     // Create huge page mapping.
    MAP_SYNC = 0x80000,  // Perform synchronous page faults for the mapping.
    MAP_FIXED_NOREPLACE =
        0x100000,  // MAP_FIXED but do not unmap underlying mapping.
};

constexpr usize page_size = 4096u;

auto mmap(usize beginning_address, usize bytes_size, usize protections,
          usize flags, isize file_descriptor, usize pages_offset)
    -> Result<void*>;

auto munmap() -> Result<>;
