// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <concepts.h>
#include <mmap.h>

struct PageAllocator {
    void* address = nullptr;

    enum failures
    {};

    auto malloc(isize) -> Result<void*> {
        this->address =
            map_memory(
                0u, 4u * page_size,
                static_cast<ProtectionFlags>(::PROT_READ | ::PROT_WRITE),
                static_cast<VisibilityFlags>(::MAP_PRIVATE | ::MAP_ANONYMOUS),
                -1, 0u)
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
