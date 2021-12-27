// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <concepts.h>
#include <mmap.h>

struct PageAllocator {
    void* address = nullptr;

    enum failures
    {};

    /* Allocate memory in multiples of a page-size,
     * which is `4` KiB on x86-64. For instance, If fewer that `4096u` bytes are
     * allocated, that amount will be rounded up to `4096u`. There is generally
     * very little reason to allocate any other amount of bytes with
     * `PageAllocator`. */
    auto malloc(usize bytes) -> Result<void*> {
        this->address = mmap(0u, bytes, ::PROT_READ | ::PROT_WRITE,
                             ::MAP_PRIVATE | ::MAP_POPULATE | ::MAP_ANONYMOUS,
                             // Anonymous pages must have `-1`.
                             -1,
                             // Anonymous pages must have `0u`.
                             0u)
                            .or_it_is(nullptr);
        if (this->address == nullptr) {
            return Failure(1);
        }
        return this->address;
    }

    void free(){};

    // NOLINTNEXTLINE
    auto operator[](isize byte_index) -> u8& {
        return reinterpret_cast<u8*>(address)[byte_index];
    }
};
