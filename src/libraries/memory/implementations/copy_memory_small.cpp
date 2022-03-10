// -*- mode: c++ -*-
// vim: set ft=cpp:
#include <string>

// NOLINTNEXTLINE clangd doesn't recognize the `gnu::optimize()` attribute.
[[gnu::optimize("-fno-tree-loop-distribute-patterns")]] void
std::copy_memory_small(void const* p_source, void* p_destination, isize bytes) {
    unsigned char const* p_source_handle =
        static_cast<unsigned char const*>(p_source);
    unsigned char* p_destination_handle =
        static_cast<unsigned char*>(p_destination);

    for (isize i = 0; i < bytes; i++) {
        p_destination_handle[i] = p_source_handle[i];
    }
}
