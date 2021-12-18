// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// Legacy numerals are required for backwards-compatibility here.
#include <stdint.h>

[[deprecated]] auto memcpy(void* destination, void const* source, size_t length)
    -> void* {
    // TODO: Even with only MMX, much better solutions should be possible.
    char const* source_iterator = static_cast<char const*>(source);
    char* destination_iterator = static_cast<char*>(destination);
    while (length > 0) {
        *destination_iterator++ = *source_iterator++;
        length--;
    }
    return destination;
}
