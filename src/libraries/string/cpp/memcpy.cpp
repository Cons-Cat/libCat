// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <string.h>

[[deprecated("memcpy() is deprecated! Use simd::copy_buffer() instead!")]] auto
memcpy(void* p_destination, void const* p_source, size_t bytes) -> void* {
    simd::copy_memory(p_source, p_destination, bytes);
    return p_destination;
}
